#include <Arduino.h>
#include <Wire.h> // I2C
#include <LittleFS.h>
#include <WiFi.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_BMP3XX.h"
#include <ESP32Servo.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>



// -------------------------------- SETTINGS & TESTS --------------------------------

// WiFi settings
const char* ssid = "orbofu";
const char* password = NULL;

// If you want to use software SPI for BMP, also edit the initBMP() function as suggested
// #define BMP_SCK 12 //13
// #define BMP_MISO 13  //12
// #define BMP_MOSI 11 //11
// #define BMP_CS 10

// Servo pin, if you want to use a servo other than the sg90, edit the initServo() function
#define Parachute_Servo_Pin 17
// Servo open & close positions in ° (open = parachute released, close = parachute still in rocket)
int parachute_servo_open_pos = 90;
int parachute_servo_close_pos = 0;

// Times
const int MAX_DATA_POINTS = 3000; // X is the max number of data points to be logged for one flight
  // Not in use, could do more harm than good
// const int MAX_TIME_PARACHUTE = ; // X ms is the max time before parachute will open after detecting launch even if apogee detection failed
const int LOOP_PERIOD = 667; // X ms is the period of the main loop - MUST BE THE SAME AS THE BMP DATA RATE PERIOD

// Margins
const float LAUNCH_MARGIN = 1.0; // X m needs to change in the positive direction for a launch to be detected
const float APOGEE_MARGIN = 2.0; // X m needs to change from the max alt for apogee detection
const float TOUCHDOWN_MARGIN = 1.0; // rocket must be X m above ground to trigger touchdown

// tests & config settings
bool test_servo = false;

// -------------------------------- DEFINITIONS --------------------------------

// Pins definition
#define ledPin D9 //onboard led, on if BMP error
#define buttonPin 27 //onboard button, use to arm flight

// BMP definition
#define SeaLevelPressure_hPa (1013.25)
Adafruit_BMP3XX bmp;

// Altimeter variables definition
float alt;
float temp;
float pres;
float abs_alt;

float start_abs_alt;
float start_temp;
float start_pres;

float apogee_alt = 0.0;
float max_alt;

// Flight and time variables definition
bool start_flight = false;
bool stop_flight = false;
bool flight_triggered = false;
bool launched = false;
bool apogee_detected = false;

unsigned int timer_abs;
unsigned int timer_relative;
unsigned int timer_start_abs;

const int MAX_TIME_LOGGING = MAX_DATA_POINTS*LOOP_PERIOD; // time in ms after which logging will stop, default 60s
bool logging = false; // True when logging data - not a setting to disable/enable data logging !

// SG90 Servo definition
Servo parachute_servo;

// AsyncWebServer server & websocket definition
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// message variable to store messages between client & server definition
String message = "";



// -------------------------------- INITILIZATIONS --------------------------------

// Initialize WiFi
void initWiFi() {
  Serial.println("\n[WiFi] Creating AP");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("[WiFi] AP Created with IP Gateway "); // Defaults to 192.168.4.1/24
  Serial.println(WiFi.softAPIP());
}

// Initialize BMP
void initBMP(){
  if (!bmp.begin_I2C()) {   // hardware I2C mode, can pass in address & alt Wire
  //if (! bmp.begin_SPI(BMP_CS)) {  // hardware SPI mode  
  //if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) {  // software SPI mode
    Serial.println("\n[BMP] Could not find a valid BMP3XX sensor, check wiring !");
    while (1);
  }
  // Set up oversampling and filter initialization
  // see oversampling & measurement sections of BMPXXX datasheet
  // possible values: https://github.com/adafruit/Adafruit_BMP3XX/blob/master/Adafruit_BMP3XX.cpp
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  // T_measure [µs] = 234 + 392 {pressure} + 163 {T} + 2020*(number of temperature & pressure measures including oversampling)
  // MUST BE THE SAME AS THE LOOP_PERIOD (frequency = 1/period)
  // can be 200,100,50,25,12_5,6_25,3_1,1_5,0_78,0_39,0_2,0_1,0_05,0_02,0_01,0_006,0_003,0_001
  bmp.setOutputDataRate(BMP3_ODR_1_5_HZ); // Defaults to 50
  Serial.println("\n[BMP] BMP Initilized !");
}

// Initialize Servo
void initServo() {
  parachute_servo.setPeriodHertz(50); // PWM frequency for SG90
  // Minimum and maximum pulse width (in µs) from 0° to 180°
  // datasheet SG90: 500,2400 | MY SG90: 800,2500
  parachute_servo.attach(Parachute_Servo_Pin, 800, 2500);
  Serial.println("\n[Servo] Servo Initilized !");
}

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("\n[LittleFS] An Error has occurred while mounting LittleFS");
  }
  else {
    Serial.println("\n[LittleFS] LittleFS mounted successfully");
  }
}



// -------------------------------- WEBSERVER FUNCTIONS --------------------------------
// See https://github.com/me-no-dev/ESPAsyncWebServer for docs

// Notifies all clients with a message
void notifyClients(String message) {
  ws.textAll(message);
}

// Serves root URL files (static files)
void serveRootURL() {
  // Server index.html when connecting
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  // Serve static files (css & js)
  server.serveStatic("/", LittleFS, "/");
}

// Handles websockets messages from client (flight_triggered "true" or "false" + confirmation to client)
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  //the whole message is in a single frame and we got all of it's data - see docs
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*) data; // converts 8-bit unsigned integers to char
    if (message == "true") {
      start_flight = true;
      Serial.println("\n[WebSocket] Confirmation Flight Armed");
    }
    else if (message == "false") {
      stop_flight = false;
      Serial.println("\n[WebSocket] Confirmation Flight Disarmed");
    }
    else {
      Serial.println("\n[WebSocket] Message received but not true neither false, ignoring");
    }
  }
}

// Handles websockets requests & passes messages to handleWebSocketMessage()
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.print("\n[WebSocket] WebSocket client ");
      // -> operator same as . operator but when using pointer
      Serial.print(client->id());
      Serial.print(" connected from ");
      Serial.println(client->remoteIP());
      // Notify client of latest apogee when it first connects
      notifyClients((String) flight_triggered);
      notifyClients((String) apogee_alt); // Not most efficient conversion compared to dtostrf but no import needed
      break;
    case WS_EVT_DISCONNECT:
      Serial.print("\n[WebSocket] WebSocket client ");
      Serial.print(client->id());
      Serial.println(" disconnected");
      break;
    case WS_EVT_DATA:
      // What to do when receiving data
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
      // Nothing to do on a pong request
      break;
    case WS_EVT_ERROR:
      Serial.println("\n[WebSocket] WebSocket error event");
      break;
  }
}



// -------------------------------- HELPER FUNCTIONS --------------------------------

// call bmp.performReading() to refresh measures
// bmp.temperature; // return temperature in °C
// bmp.pressure / 100.0F; // returns pressure in hPa

// returns relative altitude (in m) & changes pres variable
float getAltitude() {
  // BMP_3XX Barometric Formula (or Pressure Altitude Formula)
  // uses performReading() so no need to call it before to refresh 
  // return bmp.readAltitude(SeaLevelPressure_hPa) - start_abs_alt;
  if (!bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return 0.0;
  }
  // Improved version, see https://en.wikipedia.org/wiki/Barometric_formula
  pres = bmp.pressure / 100.0F;
  return (273.15 + start_temp) / 0.0065 * (1.0 - pow(pres / start_pres, 0.1903));
}

// Starts BMP sensors & updates start_ variables
void startBMP(){
  if (!bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  start_temp = bmp.temperature; // return temperature in °C
  start_pres = bmp.pressure / 100.0F; // returns pressure in hPa
  start_abs_alt = bmp.readAltitude(SeaLevelPressure_hPa); //returns approximative absolute altitude
  abs_alt = start_abs_alt;
}

// Logs in data.csv relative_time,pres,alt, (last row reserved for 1st data point with temperature)
void logData() {
  File file = LittleFS.open("/data.csv", "a");
  file.print(timer_relative);
  file.print(",");
  file.print(pres, 2);
  file.print(",");
  file.print(alt, 2);
  file.println(",,");
  file.close();
}

// Meant to be used in the loop
// Initializes flight variables & file (flushing preceding) + changes flight_triggered & notifies client
void startFlight() {
    // Initiate variables
  startBMP();
  apogee_alt = 0.0;
  max_alt = 0.0;
  launched = false;
  apogee_detected = false;
  logging = false;
  timer_abs = millis();
  timer_relative = 0;
    // File creation with start variables (incl temperature)
  File file = LittleFS.open("/data.csv", "w");
  file.println("time,pressure,altitude,temperature");
  file.print(0);
  file.print(",");
  file.print(start_pres, 2); // Print float with 2 decimal places
  file.print(",");
  file.print(0.00, 2);
  file.print(",");
  file.print(start_temp, 2);
  file.println();
  file.close();
    // Change state
  flight_triggered = true;
  digitalWrite(ledPin, HIGH);
    // Send new apogee (0.0) & flight_triggered true to client
  notifyClients((String) flight_triggered);
  delay(30);
  notifyClients((String) apogee_alt);
  Serial.println("[function check] startFlight() triggered");
}

// Meant to be used in the loop - Changes flight_triggered & notifies client
void stopFlight() {
    // Change state
  flight_triggered = false;
  digitalWrite(ledPin, LOW);
    // Send new apogee (probably 0.0) & flight_triggered false to client
  notifyClients((String) flight_triggered);
  notifyClients((String) apogee_alt);
  Serial.println("[function check] stopFlight() triggered");
}



// -------------------------------- SETUP --------------------------------

// Initialize Websocket
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(460800);
  while (!Serial);
  Serial.println("\nSetup started !");

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(ledPin, LOW);

  delay(100);
  initBMP();
  initWiFi();
  initServo();
  initLittleFS();
  initWebSocket();

  // Serves root URL (static files)
  serveRootURL();
  // Start server
  server.begin();

  delay(100);
  // BMP Tests in console
  startBMP();
  Serial.print("Start temperature = ");
  Serial.print(start_temp);
  Serial.println(" °C");
  Serial.print("Start pressure = ");
  Serial.print(start_pres);
  Serial.println(" hPa");
  Serial.print("Start approx. altitude = ");
  Serial.print(start_abs_alt);
  Serial.println(" m");
  Serial.println();

  // Servo Tests - could've been an interface button but not necessary, just need to simulate a flight by hand
  if (test_servo) {
    // test open & close servo
    parachute_servo.write(parachute_servo_open_pos);
    delay(2000);
    parachute_servo.write(parachute_servo_close_pos);
    delay(2000);
  }

  delay(5000);
  start_flight = true;
}



// -------------------------------- LOOP --------------------------------

void loop() {
  // Note: non-blocking code wasn't mandatory (still cleaner) because webserver is asynchronous

  // Init and start new flight on user demand
  if (start_flight) {
    startFlight();
    start_flight = false;
  }
  // Stop flight on user demand
  if (stop_flight) {
    stopFlight();
    stop_flight = false;
  }

  // If flight triggered
  if (flight_triggered) {
    if (millis() > timer_abs) {
      // Avoid too many checks, when to do a new measure
      timer_abs = millis()+LOOP_PERIOD;
      if (launched) {
        timer_relative = millis()-timer_start_abs; // = timer_relative+LOOP_PERIOD maybe less precise ?
      }

      // Get the altitude (& updates pres pressure value)
      float last_alt = alt;
      alt = getAltitude();
      abs_alt = bmp.readAltitude(SeaLevelPressure_hPa);
      Serial.print("[Altitude] Homemade: ");
      Serial.print(alt);
      Serial.print(" m | BMP_3XX: ");
      Serial.print(abs_alt-start_abs_alt);
      Serial.println(" m");

      // Check for a launch
      if (!launched && (alt>=LAUNCH_MARGIN)) {
        launched = true;
        logging = true; // Start logging data
        timer_start_abs = millis()-LOOP_PERIOD; // Started one iteration before
        timer_relative = LOOP_PERIOD;
        Serial.println("[main loop] Launch detected");
      }

      // Check if altitude is higher than max altitude
      if (launched && (alt >= max_alt)) {
        max_alt = alt;
      }
      // Check for apogee & trigger parachute
      if (launched && !apogee_detected && (alt <= (max_alt-APOGEE_MARGIN))) {
        apogee_detected = true;
        apogee_alt = max_alt;
        parachute_servo.write(parachute_servo_open_pos);
        Serial.println("[main loop] Apogee detected");
        Serial.print("[main loop] Apogee is ");
        Serial.print(apogee_alt);
        Serial.println(" m");
      }

      // Check if landed
      if (launched && alt<TOUCHDOWN_MARGIN && ((last_alt-alt)<0.1)){
        logging = false;
        stopFlight();
        Serial.println("[main loop] Touchdown detected");
      }

      // Calling logging function
      if (logging and timer_relative<MAX_TIME_LOGGING) {
        logData();
      }
    }
  }

  // Limits number of connected clients (disconnects oldest if too many clients)
  ws.cleanupClients();
}
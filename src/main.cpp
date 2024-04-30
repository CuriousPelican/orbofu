#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <ESP32Servo.h>



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
#define Parachute_Servo_Pin 22
// Servo open & close positions in ° (open = parachute released, close = parachute still in rocket)
int parachute_servo_open_pos = 90;
int parachute_servo_close_pos = 0;

// Times
const int MAX_DATA_POINTS = 3000; // X is the max number of data points to be logged for one flight
  // Not in use, could do more harm than good
// const int MAX_TIME_PARACHUTE = ; // X ms is the max time before parachute will open after detecting launch even if apogee detection failed
const int LOOP_PERIOD = 20; // X ms is the period of the main loop - MUST BE THE SAME AS THE BMP DATA RATE PERIOD

// Margins
const float LAUNCH_MARGIN = 1.0; // X m needs to change in the positive direction for a launch to be detected
const float APOGEE_MARGIN = 2.0; // X m needs to change from the max alt for apogee detection
const float TOUCHDOWN_MARGIN = 1.0; // rocket must be X m above ground to trigger touchdown

// tests & config settings
bool test_servo = false;

// -------------------------------- DEFINITIONS & TESTS --------------------------------

// BMP introduction
#define SeaLevelPressure_hPa (1013.25)
Adafruit_BMP3XX bmp;

// Altimeter variables
float alt;
float temp;
float pres;

float start_abs_alt;
float start_temp;
float start_pres;

float apogee_alt;
float max_alt;

// Flight and time variables
bool flight_triggered = false;
bool launched = false;
bool landed = false;
bool apogee_detected = false;

unsigned int timer_abs;
unsigned int timer_relative;
unsigned int timer_start_abs;

const int MAX_TIME_LOGGING = MAX_DATA_POINTS*LOOP_PERIOD; // time in ms after which logging will stop, default 60s
bool logging = false; // True when logging data - not a setting to disable/enable data logging !

// SG90 Servo introduction
Servo parachute_servo;



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
  // Set up oversampling and filter initialization - see oversampling & measurement sections of BMPXXX datasheet
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  // T_measure [µs] = 234 + 392 {pressure} + 163 {T} + 2020*(number of temperature & pressure measures including oversampling)
  // MUST BE THE SAME AS THE LOOP_PERIOD (frequency = 1/period)
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  Serial.println("\n[BMP] BMP Initilized !");
}

// Initialize Servo
void initServo() {
  parachute_servo.setPeriodHertz(50); // PWM frequency for SG90
  parachute_servo.attach(Parachute_Servo_Pin, 500, 2400); // Minimum and maximum pulse width (in µs) from 0° to 180°
  Serial.println("\n[Servo] Servo Initilized !");
}



// -------------------------------- HELPER FUNCTIONS --------------------------------

// bmp.temperature; // return temperature in °C
// bmp.pressure / 100.0F; // returns pressure in hPa

// returns relative altitude (in m) & changes pres variable
float getAltitude() {
  // BMP_3XX Barometric Formula (or Pressure Altitude Formula)
  // return bmp.readAltitude(SeaLevelPressure_hPa) - start_abs_alt;

  // Improved version, see https://en.wikipedia.org/wiki/Barometric_formula
  pres = bmp.pressure / 100.0F;
  return (273.15 + start_temp) / 0.0065 * (1.0 - pow(pres / start_pres, 0.1903));
}

// Logs in data.csv relative_time,pres,alt, (last row reserved for 1st data point with temperature)
void logData(File &file) {
  file.print(timer_relative);
  file.print(",");
  file.print(pres, 2);
  file.print(",");
  file.print(alt, 2);
  file.println(",,");
}



// -------------------------------- START & SETUP --------------------------------

void startBMP(){
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  start_temp = bmp.temperature; // return temperature in °C
  start_pres = bmp.pressure / 100.0F; // returns pressure in hPa
  start_abs_alt = bmp.readAltitude(SeaLevelPressure_hPa); //returns approximative absolute altitude
}

void setup() {
  Serial.begin(115200);

  initBMP();
  initWiFi();
  initServo();

  // BMP Tests
  startBMP();
  // Not using <stdio.h> for memory reasons
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

  // Servo Tests
  if (test_servo) {
    // test open & close servo
    parachute_servo.write(parachute_servo_open_pos);
    delay(2000);
    parachute_servo.write(parachute_servo_close_pos);
    delay(2000);
  }

  // LittleFS Tests
  if (!LittleFS.begin()) {
    Serial.println("[LittleFS] An Error has occurred while mounting LittleFS");
    return;
  }
}



// -------------------------------- LOOP --------------------------------

void loop() {

  if (flight_triggered) {
    // Initiate variables
    startBMP();
    apogee_alt = 0.0;
    max_alt = 0.0;
    flight_triggered = true;
    launched = false;
    landed = false;
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

    // Update to user informing flight_triggered 

    // While not landed
    while (!landed) {
      if (millis() > timer_abs) {
        // Avoid too many checks
        timer_abs = millis()+LOOP_PERIOD;
        if (launched) {
          timer_relative = millis()-timer_start_abs; // = timer_relative+LOOP_PERIOD maybe less precise ?
        }

        // Get the altitude
        float last_alt = alt;
        // float last_pres = pres;
        alt = getAltitude();

        // Check for a launch
        if (!launched && !landed && (alt>=LAUNCH_MARGIN)) {
          launched = true;
          logging = true; // Start logging data
          timer_start_abs = millis()-LOOP_PERIOD; // Started one iteration before
          timer_relative = LOOP_PERIOD;
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
        }

        // Check if landed
        if (launched && !landed && alt<TOUCHDOWN_MARGIN && ((last_alt-alt)<0.01)){
          logging = false;
          landed = true;
        }

        // Calling logging function
        if (logging and timer_relative<MAX_TIME_LOGGING) {
          logData(file);
        }
      }
    }
    
    // End loop
    file.close();
    flight_triggered = false;

    // Update to user to download & relaunch
  }
}
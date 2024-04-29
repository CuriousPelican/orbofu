#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <ESP32Servo.h>



// -------------------------------- SETTINGS --------------------------------

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

// -------------------------------- DEFINITIONS & TESTS --------------------------------

// tests
bool testServo = true;

// BMP introduction
#define SeaLevelPressure_hPa (1013.25)
Adafruit_BMP3XX bmp;

// BMP variables
float alt;
float temp;
float pres;
float start_abs_alt;
float start_temp;
float start_pres;

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
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  Serial.println("\n[BMP] BMP Initilized !");
}

// Initialize Servo
void initServo() {
  parachute_servo.setPeriodHertz(50); // PWM frequency for SG90
  parachute_servo.attach(Parachute_Servo_Pin, 500, 2400); // Minimum and maximum pulse width (in µs) from 0° to 180°
}



// -------------------------------- HELPER FUNCTIONS --------------------------------

// bmp.temperature; // return temperature in °C
// bmp.pressure / 100.0F; // returns pressure in hPa

// returns relative altitude & changes pres varaible
float getAltitude() {
  // BMP_3XX Barometric Formula (or Pressure Altitude Formula)
  // return bmp.readAltitude(SeaLevelPressure_hPa);

  // Improved version, see https://en.wikipedia.org/wiki/Barometric_formula
  pres = bmp.pressure / 100.0F;
  return (273.15 + start_temp) / 0.0065 * (1.0 - pow(pres / start_pres, 0.1903));
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

  // LittleFS tests
  // if (!LittleFS.begin()) {
  //   Serial.println("An Error has occurred while mounting LittleFS");
  //   return;
  // }

  // File file = LittleFS.open("/test.txt", "w");
  // if (!file) {
  //   Serial.println("Failed to open file for writing");
  //   return;
  // }
  // file.println("Hello, World!");
  // file.close();

  // file = LittleFS.open("/test.txt", "r");
  // if (!file) {
  //   Serial.println("Failed to open file for reading");
  //   return;
  // }
  // Serial.println("File Content:");
  // while (file.available()) {
  //   Serial.write(file.read());
  // }
  // file.close();

}



// -------------------------------- LOOP --------------------------------

void loop() {

  startBMP();

  Serial.print("Start temperature = ");
  Serial.print(start_temp);
  Serial.println(" °C");
  Serial.print("Start pressure = ");
  Serial.print(start_pres);
  Serial.println(" hPa");
  Serial.print("Sptart approx. altitude = ");
  Serial.print(start_abs_alt);
  Serial.println(" m");
  Serial.println();
  delay(1000);

  if (testServo) {
  // test open & close servo
    parachute_servo.write(parachute_servo_open_pos);
    delay(1000);
    parachute_servo.write(parachute_servo_close_pos);
    delay(1000);
  }

}
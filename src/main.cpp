#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

// BMP390L
// #define BMP_SCK 12 //13
// #define BMP_MISO 13  //12
// #define BMP_MOSI 11 //11
// #define BMP_CS 10
#define SeaLevelPressure_hPa (1013.25)
Adafruit_BMP3XX bmp;

// WiFi settings
const char* ssid = "orbofu";
const char* password = NULL;

// -------------------------- INITILIZATIONS --------------------------

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
  // T_measure (micro seconds) = 234 + 392 {pressure} + 163 {T} + 2020*(number of temperature & pressure measures including oversampling)
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  Serial.println("\n[BMP] BMP Initilized !");
}

// Initialize WiFi
void initWiFi() {
  Serial.println("\n[WiFi] Creating AP");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("[WiFi] AP Created with IP Gateway "); // Defaults to 192.168.4.1/24
  Serial.println(WiFi.softAPIP());
}

// -------------------------- HELPER FUNCTIONS --------------------------

// bmp.temperature; // return temperature in °C
// bmp.pressure; // returns pressure in Pa

void getAltitude(){
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  // BMP_3XX standart Barometric Formula (or Pressure Altitude Formula)
  // alt = bmp.readAltitude(SeaLevelPressure_hPa);

  // Improved version
}

void setup() {
  Serial.begin(115200);

  initBMP();
  initWiFi();

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

void loop() {
  if (! bmp.performReading()) {
    Serial.println("[BMP] Failed to perform reading :(");
    return;
  }
  Serial.print("Temperature = ");
  Serial.print(bmp.temperature);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bmp.pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  // Barometric Formula (or Pressure Altitude Formula)
  // Room for improvement to acheive best relative accuracy
  Serial.print(bmp.readAltitude(SeaLevelPressure_hPa));
  Serial.println(" m");

  Serial.println();
  delay(2000);
}
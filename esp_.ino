//Network  libs

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


//Sensor libs

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
​

// network defs
const char *ssid = "desktop-hot"; //Enter your WIFI ssid
const char *password = "77635Skk"; //Enter your WIFI password
const char *server_url = "http://barnibus.xyz:8080/"; // Nodejs application endpoint

WiFiClient client;

//sensor defs

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
​
#define SEALEVELPRESSURE_HPA (1013.25)
​
Adafruit_BME680 bme;


void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("BME680 test"));
  ​
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms


  Serial.println("Attempting Wifi Connection")
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("...");
  }
  Serial.println("WiFi connected");
  delay(1000);
}

void loop() {

  //read local sensor info

  Serial.print("Temperature = ");
  Serial.print(bme.temperature);
  Serial.println(" *C");
​
  Serial.print("Pressure = ");
  Serial.print(bme.pressure / 100.0);
  Serial.println(" hPa");
​
  Serial.print("Humidity = ");
  Serial.print(bme.humidity);
  Serial.println(" %");
​
  Serial.print("Gas = ");
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.println(" KOhms");
​
  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");
​
  Serial.println();

  /// Send Sensor info

  DynamicJsonDocument doc(512); //instantiate json document "doc" with 512 bytes
  doc["humidity"] = bme.temperature; // add key pair value to json ( h is key, humidity is value)
  doc["temperature"] = bme.temperature;
  doc["pressure"] = bme.pressure
  doc["gas"] = bme.gas_resistance;
  doc["altitude"] = bme.readAltitude(SEALEVELPRESSURE_HPA));

  String json;
  serializeJson(doc, json); //converst the DynamicJsonDocument into one coherent string "json"

  HTTPClient http;
  http.begin(client, server_url);
  http.addHeader("Content-Type", "application/json"); // header used to define HTTP request

  int httpCode = http.POST(json);

  if(httpCode > 0) { // 0 meaning 0 errors in response
    if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = http.getString();
      Serial.print("Response: ");Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s", http.errorToString(httpCode).c_str());
  }

  http.end();

  delay(5000);
}

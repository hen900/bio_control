#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>


#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>



// network defs
const char *ssid = "desktop-hot"; //Enter your WIFI ssid
const char *password = "77635Skk"; //Enter your WIFI password
const char *server_url = "http://barnibus.xyz:8080/"; // Nodejs application endpoint

WiFiClient client;
HTTPClient http;


SensirionI2CScd4x scd4x;

void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

void sendData(float temp, float hum, float co2){
  /// Send Sensor info

  DynamicJsonDocument doc(512); //instantiate json document "doc" with 512 bytes
  doc["humidity"] = hum; // add key pair value to json ( h is key, humidity is value)
  doc["temperature"] = temp;
  doc["co2"] = co2;

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

}

void startWifi(void){

  // network defs
const char *ssid = "desktop-hot"; //Enter your WIFI ssid
const char *password = "77635Skk"; //Enter your WIFI password
const char *server_url = "http://barnibus.xyz:8080/"; // Nodejs application endpoint

WiFiClient client;

  Serial.println("Attempting Wifi Connection");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("...");
  }
  Serial.println("WiFi connected");
  delay(1000);

}


void setup() {

    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    Serial.println("Waiting for first measurement... (5 sec)");


    startWifi();
}

void loop() {
    uint16_t error;
    char errorMessage[256];

    delay(100);

    // Read Measurement
    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    bool isDataReady = false;
    error = scd4x.getDataReadyFlag(isDataReady);
    if (error) {
        Serial.print("Error trying to execute getDataReadyFlag(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        return;
    }
    if (!isDataReady) {
        return;
    }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {
        Serial.print("Co2:");
        Serial.print(co2);
        Serial.print("\t");
        Serial.print("Temperature:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(humidity);
        sendData(temperature,humidity,co2);  // send off data
    }
}








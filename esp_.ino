#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define DHTPIN 1
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char *ssid = "desktop-hot"; //Enter your WIFI ssid
const char *password = "77635Skk"; //Enter your WIFI password
const char *server_url = "http://barnibus.xyz:8080/"; // Nodejs application endpoint

WiFiClient client;

void setup() {
  delay(3000);
  Serial.begin(9600);
  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  delay(1000);
}

void loop() {
  float h = dht.readHumidity();  // based on the DHT library, any other sensor will require a diff read function
  float t = dht.readTemperature();
  
  Serial.print("Humidity = ");
  Serial.print(h);
  Serial.print("%  ");
  Serial.print("Temperature = ");
  Serial.print(t); 
  Serial.println("C");

  DynamicJsonDocument doc(512); //instantiate json document "doc" with 512 bytes
  doc["humidity"] = h; // add key pair value to json ( h is key, humidity is value)
  doc["temperature"] = t;
  
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

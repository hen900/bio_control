#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>


#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/// actuator and sensor are made into objects
#include <Actuator.h>
#include <BioSensor.h>


// init fan actuator object
Actuator fan;

//init sensor as Biosensor object. A BioSensor reads temp,humi and co2 so you only 
//need one object
BioSensor sensor;

// network defs
const char *ssid = "desktop-hot"; //Enter your WIFI ssid
const char *password = "77635Skk"; //Enter your WIFI password
const char *server_url = "http://barnibus.xyz:8080/meas"; // Nodejs application endpoint

WiFiClient client;



//Used to send data off to endpoint 
String sendData(float temp, float hum, uint16_t co2){ // send data, returns response

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
      return payload; // this is the server response
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s", http.errorToString(httpCode).c_str());
    return http.errorToString(httpCode);
  }

  http.end();
}

void setupWifi(void){ ///simple wifi setup

    Serial.println("Attempting Wifi Connection");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print("...");
    }
    Serial.println("WiFi connected");
    delay(1000);

}


//processes the response from the server by calling acutator object functions

void processResponse( String data) {
  DynamicJsonDocument doc(512);

  // Parse the JSON string
  DeserializationError error = deserializeJson(doc, data);
  // Extract values from the JSON document
  bool atomState = doc["fanState"];
  bool fanState = doc["atomState"];

  if (fanState != fan.getState()) {
    Serial.print("fan state change ordered, setting fan to ");
    Serial.println(fanState);  
    fan.toggle();
    
    }

  
}


void setup() {

    
    setupWifi();    
    sensor.init();
    fan.init(12); // pin being used for fan ( in this case 12)
    
}

void loop() {


    sensor.read();
    float temperature = sensor.getTemperature();
    float humidity = sensor.getHumidity();
    uint16_t co2 = sensor.getCO2();
    String responseData = sendData(temperature,humidity,co2);  // send off data
    processResponse(responseData);

    delay(100);        
}








#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>


#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

/// actuator and sensor are made into objects
#include <Actuator.h>
#include <BioSensor.h>


const int refreshRate = 10; // refresh rate in seconds

// init fan actuator object
Actuator actuator1;
Actuator actuator2;
Actuator actuator3;

//init sensor as Biosensor object. A BioSensor reads temp,humi and co2 so you only 
//need one object
BioSensor sensor1;

// network defs
const char *ssid = "desktop-hot"; //Enter your WIFI ssid
const char *password = "77635Skk"; //Enter your WIFI password
const char *server_url = "http://barnibus.xyz:8080/meas"; // Nodejs application endpoint

WiFiClient client;

// NTP server 

const char* ntpServer  = "pool.ntp.org";  // NTP server address



//Used to send data off to endpoint 
String sendData(float temp, float hum, uint16_t co2, unsigned long now){ // send data, returns response

  DynamicJsonDocument doc(512); //instantiate json document "doc" with 512 bytes
  doc["humidity"] = hum; // add key pair value to json ( h is key, humidity is value)
  doc["temperature"] = temp;
  doc["co2"] = co2;
  doc["time"] = now;
  doc["actuator1Status"] =actuator1.getStatus(); // get the state of the actuator and add it to the json
  doc["actuator2Status"] = actuator2.getStatus(); // these are to be filled when a 2nd and 3rd actuator are 
  doc["actuator3Status" ] = actuator3.getStatus(); // added

  String json;
  serializeJson(doc, json); //convert the DynamicJsonDocument into one coherent string "json"

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
void setupTime(void){
  // Initialize and set the time
  configTime(0, 0, ntpServer); // UTC time; adjust the first two parameters for your time zone if needed

  // Wait until time is synchronized
  while (!time(nullptr)) {
    Serial.println("Waiting for time synchronization...");
    delay(1000);
  }
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
  // Extract new values from the JSON document
  bool newActuator1Status= doc["actuator1Set"];
  bool newActuator2Status= doc["actuator2Set"];
  bool newActuator3Status= doc["actuator3Set"];

  if (newActuator1Status != actuator1.getStatus()) {
    Serial.print(" actuator 1  state change ordered, setting to ");
    Serial.println(newActuator1Status);  
    actuator1.toggle();
    
    }

    if (newActuator2Status != actuator2.getStatus()) {
    Serial.print(" actuator 2  state change ordered, setting to ");
    Serial.println(newActuator2Status);  
    actuator2.toggle();
    
    }
  
    if (newActuator3Status != actuator3.getStatus()) {
    Serial.print(" actuator 3  state change ordered, setting to ");
    Serial.println(newActuator3Status);  
    actuator1.toggle();
    
    }
}


void setup() {
    
    setupWifi();    
    setupTime();
    sensor1.init(); // these objects are initialized earlier at top of the code
    actuator1.init(12); // pin being used for fan ( in this case 12)
}

void loop() {

    sensor1.read();
    float temperature = sensor1.getTemperature();
    float humidity = sensor1.getHumidity();
    uint16_t co2 = sensor1.getCO2();
    unsigned long now = time(nullptr);

    String responseData = sendData(temperature,humidity,co2,now);  // send off data
    processResponse(responseData);

    delay(refreshRate*1000); // refresh rate        
}








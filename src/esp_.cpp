#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Actuator.h>   
#include <BioSensor.h>

const int refreshRate = 1; // refresh rate in seconds

// init fan actuator object
Actuator actuator1;
Actuator actuator2;
Actuator actuator3;

//init sensor as Biosensor object. A BioSensor reads temp,humi and co2 so you only need one object
BioSensor sensor1;

// network defs

// //Henry Wifi
// const char *ssid = "desktop-hot"; //Enter your WIFI ssid
// const char *password = "vermont9"; //Enter your WIFI password

// //Henry Hotspot Wifi
// const char *ssid = "ipome"; //Enter your WIFI ssid
// const char *password = "poopdoop"; //Enter your WIFI password

// //Kessa Wifi
// const char *ssid = "HomeWifi"; //Enter your WIFI ssid
// const char *password = "Tonight@8"; //Enter your WIFI password

//Kessa Hotspot Wifi
const char *ssid = "kessa"; //Enter your WIFI ssid
const char *password = "noyesnonoyes"; //Enter your WIFI password

// //Alicia Wifi
// const char *ssid = "Alicia-Hotspot"; //Enter your WIFI ssid
// const char *password = "3*L5345m"; //Enter your WIFI password

//const char *server_url = "http://barnibus.xyz:8080/meas"; // Nodejs application endpoint
const char *server_url = "http://3.21.173.70:3603/meas"; // Nodejs application endpoint

WiFiClient client;

// NTP server 
const char* ntpServer  = "pool.ntp.org";  // NTP server address

//Used to send data off to endpoint 
String sendData(float temp, float hum, uint16_t co2, unsigned long now){ // send data, returns response
  Serial.print("\nIn sendData function\n");
  Serial.print("\ncurrent time is ");
  Serial.print(now);
  DynamicJsonDocument doc(512); //instantiate json document "doc" with 512 bytes
  doc["humidity"] = hum; // add key pair value to json ( h is key, humidity is value)
  doc["temperature"] = temp;
  doc["co2"] = co2;
  doc["time"] = now;
  doc["actuator1Status"] =actuator1.getStatus(); // get the state of the actuator and add it to the json
  doc["actuator2Status"] = actuator2.getStatus();  
  doc["actuator3Status" ] = actuator3.getStatus(); 

  String json;
  serializeJson(doc, json); //convert the DynamicJsonDocument into one coherent string "json"

  HTTPClient http;
  http.begin(client, server_url);
  http.addHeader("Content-Type", "application/json"); // header used to define HTTP request
  int httpCode = http.POST(json);

  //Serial.print("1E");
  if(httpCode > 0 && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) { // 0 meaning 0 errors in response
    Serial.print("\nXX\n");
    String payload = http.getString(); 
    Serial.print("Response: ");Serial.println(payload);
    return payload; // this is the server response
    
  } else {
    //Serial.printf("\n[HTTP] POST... failed, error: %s", http.errorToString(httpCode).c_str());
    return http.errorToString(httpCode);
  }

  Serial.print("1H");
  http.end();
  //Serial.print("1Z");
}

void setupTime(void) {
  Serial.print("\nIn setupTime function\n");
  // Initialize and set the time
  configTime(0, 0, ntpServer); // UTC time; adjust the first two parameters for your time zone if needed

  // Wait until time is synchronized
  while (!time(nullptr)) {
    Serial.println("Waiting for time synchronization...");
    delay(1000);
  }
}

void setupWifi(void) { ///simple wifi setup
  Serial.print("\nIn setupWifi function\n");
    delay(3000);
    Serial.println("Attempting Wifi Connection");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print("...");
    }
    Serial.println("WiFi connected");
    delay(1000);
}

void processResponse(String data) {
  Serial.print("\nIn processResponse function\n");
  Serial.println(data);
  DynamicJsonDocument doc(512);
  // Parse the JSON string
  DeserializationError error = deserializeJson(doc, data);
  if (error) {
    Serial.print("Bad deserialization");
    Serial.println(error.f_str());
    return;
  }

  // Extract new values from the JSON document
  bool newActuator1Status = doc["actuator1Status"];
  bool newActuator2Status = doc["actuator2Status"];
  bool newActuator3Status = doc["actuator3Status"];
  Serial.print("\nOld Actuator 1 Status = ");
  Serial.print(actuator1.getStatus());
  Serial.print("\nnewActuator1Status = ");
  Serial.print(newActuator1Status);

  if (newActuator1Status != actuator1.getStatus()) {
    Serial.print("\nActuator 1 state change ordered, setting to ");
    Serial.println(newActuator1Status);
    digitalWrite(10, newActuator1Status);
    //actuator1.setPin(newActuator1Status);
  }

  if (newActuator2Status != actuator2.getStatus()) {
    Serial.print(" actuator 2 state change ordered, setting to ");
    Serial.println(newActuator2Status);
    digitalWrite(11, newActuator2Status);
    //actuator2.setPin(newActuator2Status);
  }
  if (newActuator3Status != actuator3.getStatus()) {
    Serial.print(" actuator 3 state change ordered, setting to ");
    Serial.println(newActuator3Status);
    digitalWrite(12, newActuator2Status);
    //actuator3.setPin(newActuator3Status);
  }
}

void setup() {
  Serial.print("\nIn setup function\n");
  Serial.begin(115200); // Monitor for serial output

  setupWifi();    
  setupTime();
    
  sensor1.init(); // these objects are initialized earlier at top of the code 
  actuator1.init(10); 
  actuator1.init(11);
  actuator1.init(12); // pin being used for fan ( in this case 12)

  digitalWrite(10, LOW);
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
}

void loop() {
    Serial.print("\nIn loop function\n");
    //sensor1.read();
    Serial.print("\nA");
    float temperature = sensor1.getTemperature();
    Serial.print("\ntemp is ");
    Serial.print(temperature);
    float humidity = sensor1.getHumidity();
    Serial.print("\nhumidity is ");
    Serial.print(humidity);
    uint16_t co2 = sensor1.getCO2();
    Serial.print("\nco2 is ");
    Serial.print(co2);
    unsigned long now = time(nullptr);
    
    //Actuator
    actuator1.toggle();
    actuator2.toggle();
    actuator3.toggle();

    // //Turn on pins
    // digitalWrite(10, HIGH);
    // digitalWrite(11, HIGH);
    // digitalWrite(12, HIGH);

    Serial.println("\nLoop2");
    String responseData = sendData(temperature,humidity,co2,now);  // send off data
    Serial.print("\nResponse data: ");
    Serial.println(responseData);
    processResponse(responseData);
    delay(refreshRate*1000); // refresh rate        
}
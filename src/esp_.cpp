#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Actuator.h>   
#include <BioSensor.h>

//Create Actuator and Sensor Objects
Actuator actuator1;
Actuator actuator2;
Actuator actuator3;
BioSensor sensor1; 
String serverResponse; 
const int refreshRate = 10; 
int loopCounter = 0;

//Network
//const char *server_url = "http://barnibus.xyz:8080/meas"; // Nodejs application endpoint
const char *server_url = "http://3.21.173.70:3603/meas"; // Nodejs application endpoint for gwireless
WiFiClient client;
const char* ntpServer  = "pool.ntp.org";  // NTP server address

// Credentials
// //Henry Wifi
// const char *ssid = "desktop-hot"; //Enter your WIFI ssid
// const char *password = "vermont9"; //Enter your WIFI password
// //Henry Hotspot Wifi
// const char *ssid = "ipome"; //Enter your WIFI ssid
// const char *password = "poopdoop"; //Enter your WIFI password
// //Kessa Wifi
// const char *ssid = "HomeWifi"; //Enter your WIFI ssid
// const char *password = "Tonight@8"; //Enter your WIFI password
// //Kessa Hotspot Wifi
const char *ssid = "kessa"; //Enter your WIFI ssid
const char *password = "noyesnonoyes"; //Enter your WIFI password
// //Alicia Wifi
// const char *ssid = "Alicia-Hotspot"; //Enter your WIFI ssid
// const char *password = "3*L5345m"; //Enter your WIFI password

//MISC TODOS 
// figure out if I can take void out of all the function parameters 
// go home and get esp32 for testing
//test if actuator status changes on the server when I send updates in code 

void setupTime(void) {
	// //Todo: test this function and add to loop
	// // Initialize and set the time
	// configTime(0, 0, ntpServer); // UTC time; adjust the first two parameters for your time zone if needed
	// // Wait until time is synchronized
	// while (!time(nullptr)) {
	// 	Serial.println("Waiting for time synchronization...");
	// 	delay(1000);
	// }
}

void setupWifi(void) {
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

void readData(void) {
	sensor1.read();
	//this is a separate function so we can put any new sensors here (ultrasonic, float level, whatever)
	//also call camera to take a pic here
}

String makeJson(void) {
	DynamicJsonDocument doc(512); //todo static??
	String json;
	unsigned long now = time(nullptr); 
 
	doc["humidity"] = sensor1.getHumidity();
    doc["temperature"] = sensor1.getTemperature();
    doc["co2"] = sensor1.getCO2();
	doc["actuator1Status"] =actuator1.getStatus(); 
    doc["actuator2Status"] = actuator2.getStatus();  
    doc["actuator3Status" ] = actuator3.getStatus(); 
	doc["time"] = now;

	//Convert the DynamicJsonDocument into one coherent string "json"
	serializeJson(doc, json);
	return json;
}

void sendData(void){ 
	String json = makeJson();
	HTTPClient http;
	http.begin(client, server_url);
	http.addHeader("Content-Type", "application/json");
	int httpCode = http.POST(json);

	//Attempt to decode POST response (http code 0 means 0 errors)
	if(httpCode > 0 && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) { 
		serverResponse = http.getString(); 
		Serial.print("Response: "); Serial.println(serverResponse);
	} else {
		Serial.printf("\n[HTTP] POST... failed, error: %s", http.errorToString(httpCode).c_str());
		serverResponse = "error";
	}

	http.end();
}

void processResponse() {
	Serial.print("\nIn process response function\n");

	if(serverResponse == "error") { 
		Serial.print("Invalid response, no actuator updates");
		return; 
	}

	DynamicJsonDocument doc(512);
	DeserializationError error = deserializeJson(doc, serverResponse);
	if (error) {
		Serial.print("Bad deserialization");
		Serial.println(error.f_str());
		return;
	}

	// Extract new values from the JSON document
	bool newActuator1Status = doc["actuator1Status"];
	actuator1.setStatus(newActuator1Status);
	bool newActuator2Status = doc["actuator2Status"];
	actuator2.setStatus(newActuator2Status);
	bool newActuator3Status = doc["actuator3Status"];
	actuator3.setStatus(newActuator3Status);

	// //test code
	// Serial.print("\nNew actuator 1 status: ");
	// Serial.print(newActuator1Status);
	// Serial.print("\nNew actuator 2 status: ");
	// Serial.print(newActuator2Status);
	// Serial.print("\nNew actuator 3 status: ");
	// Serial.print(newActuator3Status);
}

void updateBehavior() {
	//put enviro control logic here. make decisions based on sensor values
}

void setup() {
	Serial.print("\nIn setup function\n");
	Serial.begin(115200); 

	setupWifi();    
	setupTime();
		
	sensor1.init(); 
	actuator1.init(10); 
	actuator2.init(11);
	actuator3.init(12); 

	//test code
	digitalWrite(10, LOW);
	digitalWrite(11, HIGH);
	digitalWrite(12, HIGH);
}

void loop() {
    loopCounter++;
    Serial.print("\nLoop");
    Serial.print(loopCounter);
	Serial.print("\n----------------------------------------------------------\n");

	readData();
	sendData();
	processResponse();
    updateBehavior();

    delay(refreshRate*1000); // refresh rate        
}
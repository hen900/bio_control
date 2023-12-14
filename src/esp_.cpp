#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Actuator.h>   
#include <BioSensor.h>

Actuator humidifier;
Actuator atomizer;
Actuator lights_blue;
Actuator fan_small;
Actuator fan_big;
Actuator heater;
BioSensor sensor1; 

//PIN DEFS
#define HUMIDIFIER 6
#define ATOMIZER 9
#define LIGHTS_BLUE 10
//#define LIGHTS_UV  
#define FAN_SMALL 11
#define FAN_BIG 12
#define HEATER 13

const char* ntpServer  = "pool.ntp.org";  // NTP server address for time synchronization
const long  gmtOffset_sec = -18000; //-5 hour offset for Eastern Standard Time (in seconds)
const int   daylightOffset_sec = 3600; //1 hour offset for daylight savings (in seconds)
//const char *server_url = "http://barnibus.xyz:8080/meas"; // Nodejs application endpoint
const char *server_url = "http://3.21.173.70:3603/meas"; // Nodejs application endpoint for gwireless
WiFiClient client;
String serverResponse; 
const int refreshRate = 3; 
int loopCounter = 0;

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
// move setupTime and setupWifi into synchTime and connectWifi functions with return type int, 
// handle wifi dropping better 
// sync time once a day (once a week?)
// make buffer to hold previous readings in case of dropped connection. Hold up to 10? check memory capacity for reasonable number

void setupTime() {
	// Initialize and set the time
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	// Wait until time is synchronized
	while (!time(nullptr)) {
		Serial.println("Waiting for time synchronization...");
		delay(1000);
	}
}

void setupWifi() {
	delay(3000);
	Serial.println("\nAttempting Wifi Connection to ");
    Serial.print(ssid);
	Serial.println("Network");
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print("...");
		delay(500);
	}
	Serial.println("WiFi connected");
	delay(1000);
}

void readData() {
	sensor1.read();
	//this is a separate function so we can put any new sensors here 
	//(ultrasonic, float level, whatever)
	//also call camera to take a pic here
}

String makeJson() {
	DynamicJsonDocument doc(512); //todo static??
	String json;
 
	doc["humidity"] = sensor1.getHumidity();
    doc["temperature"] = sensor1.getTemperature();
    doc["co2"] = sensor1.getCO2();
	doc["actuator1Status"] =atomizer.getStatus(); 
    doc["actuator2Status"] = lights_blue.getStatus();  
    doc["actuator3Status" ] = fan_small.getStatus(); 
	doc["actuator4Status"] = fan_big.getStatus();  
    doc["actuator5Status" ] = heater.getStatus(); 
	doc["time"] = time(nullptr);

	//Convert the DynamicJsonDocument into one coherent string "json"
	serializeJson(doc, json);
	return json;
}

void sendData(){ 
	String json = makeJson();
	HTTPClient http;
	http.begin(client, server_url);
	http.addHeader("Content-Type", "application/json");
	int httpCode = http.POST(json);

	//Attempt to decode POST response (http code 0 means 0 errors)
	if(httpCode > 0 && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) { 
		serverResponse = http.getString(); 
		//Serial.print("Response: "); Serial.println(serverResponse);
	} else {
		Serial.printf("\n[HTTP] POST... failed, error: %s", http.errorToString(httpCode).c_str());
		serverResponse = "error";
	}

	http.end();
}

void processResponse() {
	//Serial.print("\nIn process response function\n");
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

	String newActuator1Status = doc["actuator1Set"];
	String newActuator2Status = doc["actuator2Set"];
	String newActuator3Status = doc["actuator3Set"];
	String newActuator4Status = doc["actuator4Set"];
	String newActuator5Status = doc["actuator5Set"];

	atomizer.setStatus(newActuator1Status);
	lights_blue.setStatus(newActuator2Status);
	fan_small.setStatus(newActuator3Status);
	fan_big.setStatus(newActuator4Status);
	heater.setStatus(newActuator5Status);
}

void updateBehavior() {
	//put enviro control logic here. make decisions based on sensor values
}

void autonomousSequence() {
	//Phase 1
	//Turn on lights 
	lights_blue.on();
	//Turn on humidifier
	humidifier.on();
	//Turn on small fan
	fan_small.on();
	//Delay 1 min
	delay(60000);

	//Phase 2
	//Turn off lights 
	lights_blue.off();
	//Turn off humidifier
	humidifier.off();
	//Turn on small fan
	fan_small.off();
	//Turn on big fan
	fan_big.on();
	//delay 1 min(); 
	delay(60000);
	//Turn off big fan; 
	fan_big.off();
}

void setup() {
	Serial.begin(115200); 

	sensor1.init(); 
	humidifier.init(HUMIDIFIER);	//Humidifier
	atomizer.init(ATOMIZER); 		//Atomizer
	lights_blue.init(LIGHTS_BLUE);	//Lights
	fan_small.init(FAN_SMALL); 		//Small Fan
	fan_big.init(FAN_BIG); 			//Big Fan
	heater.init(HEATER); 			//Heating Pad

	autonomousSequence();
	setupWifi();    
	setupTime();
}

void loop() {

    loopCounter++;
    Serial.print("\n\nLoop");
    Serial.print(loopCounter);
	Serial.print("\n----------------------------------------------------------\n");

	readData();
	sendData();
	Serial.println("\nServer Response");
	Serial.print(serverResponse);
	processResponse();
    updateBehavior();

	Serial.println("Pin Statuses");
	Serial.println(atomizer.getStatus());
	Serial.println(lights_blue.getStatus());
	Serial.println(fan_small.getStatus());
	Serial.println(fan_big.getStatus());
	Serial.println(heater.getStatus());

    delay(refreshRate*1000);        
}

//Helpful Testing Snippets
/*
	//Fake json response for testing
	serverResponse = "{\n\t\"actuator1Set\": true,\n\t\"actuator2Set\": true,\n\t\"actuator3Set\": true\n}";

	// atomizer.on();
	// lights_blue.on();
	// fan_small.on();

*/
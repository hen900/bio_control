#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Actuator.h>   
#include <BioSensor.h>
#include <H2oSensor.h>

Actuator atomizer; 		//Actuator 1
Actuator fan; 			//Actuator 2
Actuator lights_blue; 	//Actuator 3
Actuator heater; 		//Actuator 4
Actuator lights_uv;		//Actuator 5
BioSensor bio_sensor; 
H2oSensor h2o_sensor;

//GPIO Pin Defs
#define EXTRA 15
#define ATOMIZER 14
#define FAN 32
#define LIGHTS_BLUE 27
#define HEATER 12
#define LIGHTS_UV 33 

//I2C Defs
#define CAMERA_ADDRESS 0x10
#define H20_ADDRESS 0x77

const char* ntpServer  = "pool.ntp.org";  // NTP server address for time synchronization
const long  gmtOffset_sec = -18000; //-5 hour offset for Eastern Standard Time (in seconds)
const int   daylightOffset_sec = 3600; //1 hour offset for daylight savings (in seconds)
//const char *server_url = "http://barnibus.xyz:8080/meas"; // Nodejs application endpoint
//const char *server_url = "http://mykoprisma.com:3603/setMeas"; // Nodejs application endpoint for gwireless
const char *server_url = "http://45.56.113.173:3603/setMeas"; // Nodejs application endpoint for gwireless
WiFiClient client;
String serverResponse; 
const int refreshRate = 5; 
int loopCounter = 0;
unsigned long startTime = 0;
const unsigned long timeout = 60000; // Wifi timeout duration in milliseconds (1 min)

//Targets - initialized with reasonable starting values
double targetHumidityH = 60;
double targetHumidityL = 40;
double targetCo2H = 600;
double targetCo2L = 300;
double targetTemperatureH = 25;
double targetTemperatureL = 15;

// Credentials
// //Henry Wifi
// const char *ssid = "desktop-hot"; //Enter your WIFI ssid
// const char *password = "vermont9"; //Enter your WIFI password
// //Henry Hotspot Wifi
// const char *ssid = "ipome"; //Enter your WIFI ssid
// const char *password = "poopdoop"; //Enter your WIFI password
// //Henry Laptop Hotspot
const char *ssid = "ghost"; //Enter your WIFI ssid
const char *password = "thewinds"; //Enter your WIFI password
// //Kessa Wifi
// const char *ssid = "HomeWifi"; //Enter your WIFI ssid
// const char *password = "Tonight@8"; //Enter your WIFI password
// //Kessa Hotspot Wifi
// const char *ssid = "kessa"; //Enter your WIFI ssid
// const char *password = "noyesnonoyes"; //Enter your WIFI password
// //Alicia Wifi
// const char *ssid = "Alicia-Hotspot"; //Enter your WIFI ssid
// const char *password = "3*L5345m"; //Enter your WIFI password

// MISC TODOS 
// sync time once a day (once a week?)
// make buffer to hold previous readings in case of dropped connection. Hold up to 10? check memory capacity for reasonable number

void syncTime() {
	// Initialize and set the time
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

	// Wait until time is synchronized
	while (!time(nullptr)) {
		if (millis() - startTime >= timeout) {
			Serial.println("Time sync timed out");
			return;
		}
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

void checkWifi() {
	if (WiFi.status() == WL_CONNECTED) { return; }

	startTime = millis(); // Start the timer

	while (WiFi.status() != WL_CONNECTED) {
		if (millis() - startTime >= timeout) {
			Serial.println("WiFi connection timed out");
			return;
		}
		
		Serial.print("...");
		delay(500);
	}
	Serial.println("WiFi connected");
	delay(1000);
}

void readData() {
	bio_sensor.read();
	h2o_sensor.getWaterLevel();
	//takePicture();
}

String makeJson() {
	DynamicJsonDocument doc(512); //todo static??
	String json;
 
	doc["humidity"] = bio_sensor.getHumidity();
	doc["co2"] = bio_sensor.getCO2();
    doc["temperature"] = bio_sensor.getTemperature();
	doc["timestamp"] = time(nullptr);
	doc["waterLevel"] = h2o_sensor.getWaterLevel();
	doc["actuator0Status"] = lights_blue.getStatus();
	doc["actuator1Status"] =atomizer.getStatus(); 
    doc["actuator2Status"] = lights_blue.getStatus();  
    doc["actuator3Status" ] = lights_uv.getStatus(); 
	doc["actuator4Status"] = fan.getStatus();  
    doc["actuator5Status" ] = heater.getStatus();
	
	//Convert the DynamicJsonDocument into one coherent string "json"
	serializeJson(doc, json);

	Serial.print("\nJson: ");
	Serial.print(json);

	return json;
}

void sendData(){ 
	String json = makeJson();
	Serial.print("\nSending message to server: ");
	Serial.print(json);
	Serial.println("\nEnd of message");

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

	// Grab new values from server response
	targetHumidityH = doc["presetHumidityH"].as<double>();
	targetHumidityL = doc["presetHumidityL"].as<double>();
	targetCo2H = doc["presetCo2H"].as<double>();
	targetCo2L = doc["presetCo2L"].as<double>();
	targetTemperatureH = doc["presetTemperatureH"].as<double>();
	targetTemperatureL = doc["presetTemperatureL"].as<double>();	
	
	// Grab override values from server response
	atomizer.override = doc["actuator1Override"].as<int>();
	fan.override = doc["actuator2Override"].as<int>();
	lights_blue.override = doc["actuator3Override"].as<int>();
	heater.override = doc["actuator4Override"].as<int>();
	lights_uv.override = doc["actuator5Override"].as<int>();
}

void updateBehavior() {
	//Update actuator behavior based on sensor readings and target values
	//Get latest sensor readings
	readData();

	//Atomizer
	if (atomizer.override == 2) {
		if (bio_sensor.getHumidity() < targetHumidityL) { atomizer.on(); } 
		else if (bio_sensor.getHumidity() > targetHumidityH) { atomizer.off(); }
	} else { atomizer.setStatus(atomizer.override); }

	//Fan
	if (fan.override == 2) {
		if (bio_sensor.getCO2() > targetCo2H) { fan.on(); } 
		else if (bio_sensor.getCO2() < targetCo2L) { fan.off(); }
	} else { fan.setStatus(fan.override); }

	//Lights Blue
	if (lights_blue.override == 2) {
		//Add lights logic later
	} else { lights_blue.setStatus(lights_blue.override); }

	//Heater
	if (heater.override == 2) {
		if (bio_sensor.getTemperature() < targetTemperatureL) { heater.on(); } 
		else if (bio_sensor.getTemperature() > targetTemperatureH) { heater.off(); }
	} else { heater.setStatus(heater.override); }

	//Lights UV
	if (lights_uv.override == 2) {
		//Add lights logic later
	} else { lights_uv.setStatus(lights_uv.override); }
}

void takePicture() {
	Wire.beginTransmission(CAMERA_ADDRESS); 
	Wire.write(0x01);
	Wire.endTransmission();
}

void autonomousSequence() {
	// lights_blue.on();
	// atomizer.on();
	// fan_small.on();
	// fan_big.on();
	// heater.on();

	//Phase 1: Turn on lights, humidifier, and small fan for
	lights_blue.on();
	atomizer.on();
	//fan_small.on();

	//Establish original values
	readData();
	float originalHumidity = bio_sensor.getHumidity();
	float newHumidity = 0.0; 

	// while (originalHumidity == 0.0) {
	// 	Serial.println("Attempting to read new humidity...");
	// 	originalHumidity = sensor1.getHumidity();
	// 	delay(5000);
	// }
	
	Serial.print("Original humidity is: ");
	Serial.println(originalHumidity);
	delay(60000);

	// //Wait for humidity to increase by 50%
	// Serial.println("\nWaiting for humidity to increase by 50%");
	// while(newHumidity < (originalHumidity * 1.5)) {
	// 	delay(10000);
	// 	readData();
	// 	sendData();
	// 	newHumidity = sensor1.getHumidity();
	// }

	//Phase 2: Turn off lights, humidifier, and small fan; clear the humidity
	lights_blue.off();
	atomizer.off();
	//fan_small.off();
	fan.on();
	delay(60000);

	//Wait for humidity to decrease by 50%
	Serial.println("\nWaiting for humidity to decrease by 50%");
	while(newHumidity > originalHumidity) {
		delay(10000);
		readData();
		sendData();           
		newHumidity = bio_sensor.getHumidity();
	}

	fan.off();
	Serial.print("Autonomous cycle complete");
}

void toggleAll(){
	atomizer.toggle();
	fan.toggle();
	lights_blue.toggle();
	heater.toggle();
	lights_uv.toggle();
}

void blinkInOrder() {
	atomizer.on();
	delay(3000);
	atomizer.off();
	delay(3000);
	lights_blue.on();
	delay(3000);
	lights_blue.off();
	delay(3000);
	lights_uv.on();
	delay(3000);
	lights_uv.off();
	delay(3000);
	fan.on();
	delay(3000);
	fan.off();
	heater.on();
	delay(3000);
	heater.off();
	delay(3000);
}

void i2cScanner() {
	byte error, address;
	int nDevices;

	Serial.println("Scanning...");

	nDevices = 0;
	for(address = 1; address < 127; address++ ) {
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0) {
			Serial.print("I2C device found at address 0x");
			if (address<16) 
				Serial.print("0");
			Serial.print(address,HEX);
			Serial.println("  !");
			nDevices++;
		} else if (error==4) {
			Serial.print("Unknown error at address 0x");
			if (address<16) 
				Serial.print("0");
			Serial.println(address,HEX);
		}
	}
	if (nDevices == 0)
		Serial.println("No I2C devices found\n");
	else
		Serial.println("done\n");
}

void setup() {
	Serial.begin(115200); 
	Wire.begin();

	bio_sensor.init(); 
	atomizer.init(ATOMIZER);
	lights_blue.init(LIGHTS_BLUE);
	lights_uv.init(LIGHTS_UV);
	fan.init(FAN);
	heater.init(HEATER);

	setupWifi();    
	syncTime();
}

void loop() {
    loopCounter++;
    Serial.print("\n\nLoop");
    Serial.print(loopCounter);
	Serial.print("\n----------------------------------------------------------\n");
	
	checkWifi();
	readData();
	sendData();
	processResponse();
	updateBehavior();

	//toggleAll();
	//blinkInOrder();

	Serial.println("\nStatuses");
	Serial.print ("Atomizer: ");
	Serial.println(atomizer.getStatus());
	Serial.print("Fan: ");
	Serial.println(fan.getStatus());
	Serial.print("Lights Blue: ");
	Serial.println(lights_blue.getStatus());
	Serial.print("Heater: ");
	Serial.println(heater.getStatus());
	Serial.print("Lights UV: ");
	Serial.println(lights_uv.getStatus());

    delay(refreshRate*1000);   

	// Serial.print("\nCalling take picture function");
	// takePicture();
	// Serial.println("Picture taken");     
} 

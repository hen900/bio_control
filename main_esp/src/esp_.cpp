#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Actuator.h>   
#include <BioSensor.h>
#include <H2oSensorCapacitive.h>
#include <H2oSensorResistive.h>

Actuator extra; 		//Actuator 0
Actuator atomizer; 		//Actuator 1
Actuator fan; 			//Actuator 2
Actuator lights_blue; 	//Actuator 3
Actuator heater; 		//Actuator 4
Actuator lights_uv;		//Actuator 5

BioSensor bio_sensor; 
H2oSensorResistive h2o_sensor;

//GPIO Pin Defs
#define EXTRA 15
#define ATOMIZER 14
#define FAN 32
#define LIGHTS_BLUE 27
#define HEATER 12
#define LIGHTS_UV 33 
#define WATER_ANALOG_DATA 36
#define WATER_ANALOG_POWER 13

//I2C Defs
#define CAMERA_ADDRESS 0x05
#define H2O_ADDRESS 0x77

//Constants
#define LOW_WATER_THRESHOLD 15
#define CONNECTION_TIMEOUT 10000

//Wifi
const char *ssid[] = {"olivia", "sabrina", "taylor", "kessa"};
const char *password[] = {"squidward", "squidward", "squidward", "noyesnonoyes"};

//Networking Info
const char* ntpServer  = "pool.ntp.org";  // NTP server address for time synchronization
const long  gmtOffset_sec = -18000; //-5 hour offset for Eastern Standard Time (in seconds)
const int   daylightOffset_sec = 3600; //1 hour offset for daylight savings (in seconds)
const char *server_url = "http://45.56.113.173:3603/setMeas"; // Nodejs application endpoint for gwireless
WiFiClient client;
String serverResponse; 
int refreshRate = 10; 
int loopCounter = 0;

//Targets - initialized with reasonable starting values
double targetHumidityH = 60;
double targetHumidityL = 40;
double targetCo2H = 700;
double targetCo2L = 400;
double targetTemperatureH = 25;
double targetTemperatureL = 15;

//Test Var
bool is_calibrated = false; 

void syncTime() {
    // Set the time zone
	unsigned long start_time = millis();
    const char* timeZone = "EST5EDT,M3.2.0,M11.1.0";  // Eastern Time Zone (US)
    configTzTime(timeZone, ntpServer);
    // Wait until time is synchronized
    while (!time(nullptr)) {
        if (millis() - start_time >= CONNECTION_TIMEOUT) {
            Serial.println("Time sync timed out");
            return;
        }
        Serial.println("Waiting for time synchronization...");
        delay(1000);
    }
	Serial.println("Time synchronized");
}

int setupWifi() {
	delay(3000);
	unsigned long start_time;
	int ssid_length = sizeof(ssid) / sizeof(ssid[0]);

	for(int i = 0; i < ssid_length; i++) {
		Serial.println("\nAttempting Wifi Connection to ");
		Serial.print(ssid[i]);
		Serial.println("Network");
		start_time = millis();
		WiFi.begin(ssid[i], password[i]);


		while (WiFi.status() != WL_CONNECTED) {
			if (millis() - start_time >= CONNECTION_TIMEOUT) {
				Serial.println("WiFi connection timed out");
				break;
			}
			Serial.print("...");
			delay(500);
		}
		
		if(WiFi.status() == WL_CONNECTED) {
			Serial.println("WiFi connected");
			delay(1000);
			return 1;
		}
	}

	Serial.println("Failed to connect to any network");
	return 0;
}

int checkWifi() {
	if (WiFi.status() == WL_CONNECTED) { return 1; }

	delay(3000);
	unsigned long start_time = millis();
	int ssid_length = sizeof(ssid) / sizeof(ssid[0]);

	for(int i = 0; i < ssid_length; i++) {
		Serial.println("\nAttempting Wifi Connection to ");
		Serial.print(ssid[i]);
		Serial.println("Network");
		start_time = millis();
		WiFi.begin(ssid[i], password[i]);


		while (WiFi.status() != WL_CONNECTED) {
			if (millis() - start_time >= CONNECTION_TIMEOUT) {
				Serial.println("WiFi connection timed out");
				break;
			}
			Serial.print("...");
			delay(500);
		}
		if(WiFi.status() == WL_CONNECTED) {
			Serial.println("WiFi connected");
			delay(1000);
			return 1;
		}
	}
	Serial.println("Failed to connect to any network");
	return 0;
}

// void calibrateSensor() {
// 	//check sensor every 1 day and 5 minutes
// 	if(millis() > 200000 && is_calibrated == 0) { //--
// 		if(bio_sensor.calibrate(targetCo2L)) {
// 			Serial.println("Sensor calibrated");
// 			is_calibrated = true;
// 		} else {
// 			Serial.println("Sensor calibration failed");
// 			is_calibrated = false;
// 		}	//--
// 	 } else if (millis() % 86400000 == 300000) {
// 		if(bio_sensor.calibrate(targetCo2L)) {
// 			Serial.println("Sensor calibrated");
// 			is_calibrated = true;
// 		} else {
// 			Serial.println("Sensor calibration failed");
// 			is_calibrated = false;
// 		}
// 	}
// }

void readData() {
	bio_sensor.read();
	// h2o_sensor.read();
	//takePicture();
}

int getTime() {
	int timestamp = time(nullptr);
	if (timestamp < 10000) { //sanity check for timestamp
		Serial.println("Failed to obtain time");
		return 0;
	} else {
		return timestamp; //return time in seconds since 1970
	}
}	

String makeJson() {
	DynamicJsonDocument doc(512); //todo static??
	String json;
 
	doc["humidity"] = bio_sensor.getHumidity();
	doc["co2"] = bio_sensor.getCO2();
    doc["temperature"] = bio_sensor.getTemperature();
	doc["timestamp"] = getTime();
	doc["waterLevel"] = h2o_sensor.read();
	doc["actuator0Status"] = lights_blue.getStatus();
	doc["actuator1Status"] =atomizer.getStatus(); 
    doc["actuator2Status"] = lights_blue.getStatus();  
    doc["actuator3Status" ] = lights_uv.getStatus(); 
	doc["actuator4Status"] = fan.getStatus();  
    doc["actuator5Status" ] = heater.getStatus();
	
	//Convert the DynamicJsonDocument into one coherent string "json"
	serializeJson(doc, json);
	return json;
}

void sendData(){ 
	//Avoid system crash if no wifi connection
	if (WiFi.status() != WL_CONNECTED) { 
		Serial.println("No wifi connection, skipping data send");
		return; 
	}

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
		Serial.print("Response: "); Serial.println(serverResponse);
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

	//Update refresh rate
	refreshRate = doc["updateInterval"].as<int>();
}

void updateBehavior() {
	//Update actuator behavior based on sensor readings and target values
	//Get latest sensor readings
	readData();

	// extra.setStatus(1);
	// fan.setStatus(1);
	// lights_blue.setStatus(1);
	// lights_uv.setStatus(1);
	// heater.setStatus(0);
	// atomizer.setStatus(0);

	//Lights Blue
	if (lights_blue.override == 2) {
		lights_blue.on();
	} else { lights_blue.setStatus(lights_blue.override); }

	//Heater
	if (heater.override == 2) {
		if (bio_sensor.getTemperature() < targetTemperatureL) { heater.on(); } 
		else if (bio_sensor.getTemperature() > targetTemperatureH) { heater.off(); }
	} else { heater.setStatus(heater.override); }

	//Lights UV
	if (lights_uv.override == 2) {
		lights_uv.off();
	} else { lights_uv.setStatus(lights_uv.override); }

	//Atomizer
	if ((atomizer.override == 2) && (h2o_sensor.read() > LOW_WATER_THRESHOLD)) {
		if (bio_sensor.getHumidity() < targetHumidityL) { atomizer.on(); } 
		else if (bio_sensor.getHumidity() > targetHumidityH) { atomizer.off(); }
	} else if (h2o_sensor.read() > 10) {
		atomizer.setStatus(atomizer.override); 
		heater.off();
		fan.off();
	} else {
		atomizer.off();
		heater.off();
		fan.off();
	}

	//Fan - must be evaluated after atomizer
	if (fan.override == 2 && atomizer.getStatus() == 0){
		if (bio_sensor.getCO2() > targetCo2H) { fan.on(); } 
		else if (bio_sensor.getCO2() < targetCo2L) { fan.off(); }
	} else { fan.setStatus(fan.override); }
}

void autonomousSequence() {
	// lights_blue.on();
	// atomizer.on();
	// fan_small.on();
	// fan_big.on();
	// heater.on();

	//Phase 1: Turn on lights and humidifier
	lights_blue.on();
	atomizer.on();

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
	h2o_sensor.init(WATER_ANALOG_DATA, WATER_ANALOG_POWER);
	atomizer.init(ATOMIZER);
	lights_blue.init(LIGHTS_BLUE);
	lights_uv.init(LIGHTS_UV);
	fan.init(FAN);
	heater.init(HEATER);

	lights_blue.on();
	lights_uv.on();
	atomizer.off();

	setupWifi();  
	syncTime();	
}

void loop() {
    loopCounter++;
    Serial.print("\n\nLoop");
    Serial.print(loopCounter);
	Serial.print("\n----------------------------------------------------------\n");
	
	checkWifi();
	//calibrateSensor();
	readData();
	sendData();
	processResponse();
	updateBehavior();

	Serial.println("\nStatuses");
	Serial.print("Extra: ");
	Serial.println(extra.getStatus());
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
	Serial.print("Water Level: ");
	Serial.print(h2o_sensor.read());
	Serial.println("%");

	Serial.print("\nTimestamp: ");
	Serial.println(time(nullptr));
	// Serial.print("Is calibrated: ");
	// Serial.println(is_calibrated);

    delay(refreshRate*2000);   //todo restore to 1000 after server correction

	// i2cScanner();
	// Serial.println("\nCalling take picture function");
	// takePicture();    
} 

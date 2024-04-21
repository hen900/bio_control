#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <esp_camera.h>
#include <esp_app_trace.h>

// ESP32-CAM (AI-Thinker model) pins configuration
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Constants
#define CONNECTION_TIMEOUT 10000

//Wifi
const char *ssid[] = {"olivia", "sabrina", "taylor", "kessa"}; 
const char *password[] = {"squidward", "squidward", "squidward", "noyesnonoyes"};

//Network Info
const char* ntpServer  = "pool.ntp.org";  // NTP server address for time synchronization
const char* serverAddress = "mykoprisma.com";
const int serverPort = 3603;
unsigned long startTime = 0;

//Debugging
unsigned long free_initial_heap;
unsigned long free_setup_heap;

void syncTime() {
    // Set the time zone
    unsigned long start_time = millis();
    const char* timeZone = "EST5EDT,M3.2.0,M11.1.0";  // Eastern Time Zone (US)
    configTzTime(timeZone, ntpServer);
    
    // Wait until time is synchronized
    while (!time(nullptr)) {
        if (millis() - startTime >= CONNECTION_TIMEOUT) {
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
	int ssid_length = sizeof(ssid) / sizeof(ssid[0]);

	for(int i = 0; i < ssid_length; i++) {
		Serial.println("\nAttempting Wifi Connection to ");
		Serial.print(ssid[i]);
		Serial.println("Network");
		unsigned long start_time = millis();
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
    unsigned long start_time = 0;
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

void setupCamera() {
    // Camera configuration
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // Init Camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
}

void sendPicture() {
    // Capture and discard the first 3 images to allow camera to stabilize
    for (int i = 0; i < 3; i++) {
        camera_fb_t* fb = esp_camera_fb_get();
        esp_camera_fb_return(fb);
        delay(1000);
    }

    // Capture image  
    camera_fb_t* fb = esp_camera_fb_get();

    
    if (!fb) {
        esp_camera_deinit();
        delay(1000);  // Optional delay to allow hardware to reset
        setupCamera();
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed, reinit did not resolve the issue");
            return;
        }
    }

    Serial.println("Image captured");

    //Prevent system crash from wifi connection failure
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        esp_camera_fb_return(fb);
        return;
    }

    //Send image to server
    HTTPClient http;
    http.begin(serverAddress, serverPort, "/setPhoto");
    // Set headers
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("Content-Length", String(fb->len));
    int httpResponseCode = http.POST(fb->buf, fb->len);
    if (httpResponseCode > 0) {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
        Serial.printf("HTTP Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
    esp_camera_fb_return(fb);
}

void setup() {
    Serial.begin(115200);
    free_initial_heap = esp_get_free_heap_size();
    Serial.print("Free initial heap: ");
    Serial.println(free_initial_heap);

    syncTime();
    setupWifi();
    setupCamera();

    Serial.println("Free heap after setup");
    free_setup_heap = esp_get_free_heap_size();
    Serial.print(free_setup_heap*100/free_initial_heap);
    Serial.println("%");
}

void loop() {
    //Memory leak sanity check
    Serial.print("\nFree heap in loop: ");
    Serial.print(esp_get_free_heap_size()*100/free_initial_heap);
    Serial.print("%, ");
    Serial.print(esp_get_free_heap_size()*100/free_setup_heap);
    Serial.println("%");  

    sendPicture();
    checkWifi();
    delay(10000); // Wait for 10 seconds before the next capture (switch to ten minutes when hotspot is fixed)
}

//Todo process HTTP responses
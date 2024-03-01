#include "esp_camera.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include <ESP_Mail_Client.h>
#include <FS.h>
#include <WiFi.h>
#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Wire.h>

// Network Info
// //Kessa Hotspot
// const char *ssid = "kessa"; //Enter your WIFI ssid
// const char *password = "noyesnonoyes"; //Enter your WIFI password
// //Henry Laptop Hotspot
const char *ssid = "ghost"; //Enter your WIFI ssid
const char *password = "thewinds"; //Enter your WIFI password

const int serverPort = 80;
WiFiClient client;
String serverName = "mykologic.com";
String serverPath = "/upload2.php"; 

//NTP time values 
const char* ntpServer  = "pool.ntp.org";  // NTP server address for time synchronization
const long  gmtOffset_sec = -18000; //-5 hour offset for Eastern Standard Time (in seconds)
const int   daylightOffset_sec = 3600; //1 hour offset for daylight savings (in seconds)

//Timer Values
const int timerInterval = 10000;    // time between each HTTP POST image
unsigned long previousMillis = 0;   // last time image was sent

// To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
#define emailSenderAccount    "mykologic.project@gmail.com"
#define emailSenderPassword   "rwzvbujmecaiacdr"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "Sent From ESP32"
#define emailRecipient        "kessa.crean@gmail.com"

// Photo File Name to save in LittleFS
#define FILE_PHOTO "photo.jpg"
#define FILE_PHOTO_PATH "/photo.jpg"

#define CAMERA_MODEL_AI_THINKER
#if defined(CAMERA_MODEL_AI_THINKER)
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
#else
  	#error "Camera model not selected"
#endif

SMTPSession smtp;		//The SMTP Session object used for Email sending
void smtpCallback(SMTP_Status status);	//Callback function to get the Email sending status

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

void setupCam() {
	//Todo transfer from main setup
}

void capturePhotoSaveLittleFS() {

	// Dispose first pictures because of bad quality
	camera_fb_t* fb = NULL;
	for (int i = 0; i < 3; i++) {
		fb = esp_camera_fb_get();
		esp_camera_fb_return(fb);
		fb = NULL;
  	}
    
	// Take a new photo
	fb = NULL;  
	fb = esp_camera_fb_get();  
	if(!fb) {
		Serial.println("Camera capture failed");
		delay(1000);
		ESP.restart();
	}  

	// Photo file name
	Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
	File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

	// Insert the data in the photo file
	if (!file) {
			Serial.println("Failed to open file in writing mode");
	}
	else {
		file.write(fb->buf, fb->len); // payload (image), payload length
		Serial.print("The picture has been saved in ");
		Serial.print(FILE_PHOTO_PATH);
		Serial.print(" - Size: ");
		Serial.print(fb->len);
		Serial.println(" bytes");
	}
	// Close the file
	file.close();
	esp_camera_fb_return(fb);
}

String postJPGPhoto() { //last photo
	String getAll;
	String getBody;

	camera_fb_t * fb = NULL;
	fb = esp_camera_fb_get();	//what is this line doing??? 
	if(!fb) {
		Serial.println("Camera capture failed");
		esp_camera_fb_return(fb);
		delay(1000);
		ESP.restart();
	}
	
	Serial.print("Connecting to server: ");
	Serial.println(serverName);

	if (client.connect(serverName.c_str(), serverPort)) {
		Serial.println("Connection successful!");    
		String head = "--image\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"mushroomPic.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
		String tail = "\r\n--image--\r\n";

		uint32_t imageLen = fb->len;
		uint32_t extraLen = head.length() + tail.length();
		uint32_t totalLen = imageLen + extraLen;

		Serial.println("\nAttempting post");
		client.print("POST "); 
		client.print(serverPath); 
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(serverName);
		client.print("Content-Length: ");
		client.println(String(totalLen));
		client.println("Content-Type: multipart/form-data; boundary=image");
		client.println();
		client.print(head);

		Serial.println("\nAttempting to write photo");
		uint8_t *fbBuf = fb->buf;
		size_t fileLen = fb->len;
		for (size_t n=0; n<fileLen; n=n+1024) {
		if (n+1024 < fileLen) {
			client.write(fbBuf, 1024);
			fbBuf += 1024;
		}
		else if (fileLen%1024>0) {
			size_t remainder = fileLen%1024;
			client.write(fbBuf, remainder);
		}
		}   
		client.print(tail);
		Serial.println("\nAFinished Post");
		
		esp_camera_fb_return(fb);
		
		int timoutTimer = 10000;
		long startTimer = millis();
		boolean state = false;
		
		while ((startTimer + timoutTimer) > millis()) {
		Serial.print(".");
		delay(100);      
		while (client.available()) {
			char c = client.read();
			if (c == '\n') {
			if (getAll.length()==0) { state=true; }
			getAll = "";
			}
			else if (c != '\r') { getAll += String(c); }
			if (state==true) { getBody += String(c); }
			startTimer = millis();
		}
		if (getBody.length()>0) { break; }
		}
		Serial.println();
		client.stop();
		Serial.println(getBody);
	} else {
		esp_camera_fb_return(fb);
		Serial.print("Connection to ");
		Serial.print(serverName);
		Serial.println(" failed.");
	}
	return getBody;
}

void sendPhotoEmail( void ) {
	smtp.debug(1); //Enable debug in serial port (1 = debug, 0 = no feedback)
	smtp.callback(smtpCallback);

	Session_Config config;	//Declare the session config data TODO figure out what this means???
	
	/*Set the NTP config time
	For times east of the Prime Meridian use 0-12
	For times west of the Prime Meridian add 12 to the offset.
	Ex. American/Denver GMT would be -6. 6 + 12 = 18
	See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
	*/
	config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
	config.time.gmt_offset = 0;
	config.time.day_light_offset = 1;

	/* Set the session config */
	config.server.host_name = smtpServer;
	config.server.port = smtpServerPort;
	config.login.email = emailSenderAccount;
	config.login.password = emailSenderPassword;
	config.login.user_domain = "";

	/* Declare the message class */
	SMTP_Message message;

	/* Enable the chunked data transfer with pipelining for large message if server supported */
	message.enable.chunking = true;

	/* Set the message headers */
	message.sender.name = "ESP32-CAM";
	message.sender.email = emailSenderAccount;

	message.subject = emailSubject;
	message.addRecipient("Kessa", emailRecipient);

	String htmlMsg = "<h2>New photo captured with ESP32-CAM.</h2>";
	message.html.content = htmlMsg.c_str();
	message.html.charSet = "utf-8";
	message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

	message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;
	message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

	/* The attachment data item */
	SMTP_Attachment att;

	/** Set the attachment info e.g. 
	 * file name, MIME type, file path, file storage type,
	 * transfer encoding and content encoding
	 */
	att.descr.filename = FILE_PHOTO;
	att.descr.mime = "image/png"; 
	att.file.path = FILE_PHOTO_PATH;
	att.file.storage_type = esp_mail_file_storage_type_flash;
	att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

	/* Add attachment to the message */
	message.addAttachment(att);

	/* Connect to server with the session config */
	if (!smtp.connect(&config))
		return;

	/* Start sending the Email and close the session */
	if (!MailClient.sendMail(&smtp, &message, true)) {
		Serial.print("Error sending Email, " );
		Serial.println(smtp.errorReason());
	}
}

// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status){

	Serial.println(status.info());

	if (status.success())
	{
		Serial.println("----------------");
		Serial.printf("Message sent success: %d\n", status.completedCount());
		Serial.printf("Message sent failled: %d\n", status.failedCount());
		Serial.println("----------------\n");
		struct tm dt;

		for (size_t i = 0; i < smtp.sendingResult.size(); i++){
		/* Get the result item */
		SMTP_Result result = smtp.sendingResult.getItem(i);
		time_t ts = (time_t)result.timestamp;
		localtime_r(&ts, &dt);

		ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
		ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
		ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
		ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
		ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
		}
		Serial.println("----------------\n");

	// You need to clear sending result as the memory usage will grow up.
	smtp.sendingResult.clear();
	}
}

void setup() {
	//TODO test removing brownout detection. From sample code
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
	
	Serial.begin(115200); 
	setupTime();
	setupWifi();
	setupCam();
	ESP_MAIL_DEFAULT_FLASH_FS.begin();

	//Todo: Move here onward into setupCam block after everything else has been confirmed to work physically
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
	config.pin_sccb_sda = SIOD_GPIO_NUM;
	config.pin_sccb_scl = SIOC_GPIO_NUM;
	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;
	config.xclk_freq_hz = 20000000;
	config.pixel_format = PIXFORMAT_JPEG;
	config.grab_mode = CAMERA_GRAB_LATEST;
	
	if(psramFound()){
		config.frame_size = FRAMESIZE_UXGA;
		config.jpeg_quality = 40;
		config.fb_count = 1;
	} else {
		config.frame_size = FRAMESIZE_SVGA;
		config.jpeg_quality = 60;
		config.fb_count = 1;
	}

	// Initialize camera
	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK) {
		Serial.printf("Camera init failed with error 0x%x", err);
		return;
	}
}

void checkStatus() {
	//Make sure wifi is still connected
	//Make sure time is still synced 
}

void loop() {
	//GENERAL TODO
	//Wire button to an io pin that will trigger image capture and post to server 
	//Set up as I2C slave

	checkStatus();
	capturePhotoSaveLittleFS();
	postJPGPhoto();
	//sendPhotoEmail();
	delay(10000);
}


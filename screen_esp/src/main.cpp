
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// This sketch if for an ESP32, it draws Jpeg images pulled from an SD Card
// onto the TFT.

// As well as the TFT_eSPI library you will need the JPEG Decoder library.
// A copy can be downloaded here, it is based on the library by Makoto Kurauchi.
// https://github.com/Bodmer/JPEGDecoder

// Images on SD Card must be put in the root folder (top level) to be found
// Use the SD library examples to verify your SD Card interface works!

// The example images used to test this sketch can be found in the library
// JPEGDecoder/extras folder
//----------------------------------------------------------------------------------------------------

#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <WiFi.h>

#define CONNECTION_TIMEOUT 10000

TFT_eSPI tft = TFT_eSPI();

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

// Function prototypes
void drawSdJpeg(const char *filename, int xpos, int ypos);
void jpegRender(int xpos, int ypos);
void jpegInfo();
void showTime(uint32_t msTime);
void drawFahrenheit(int number);
void drawCelcius(int number);
void drawHumidity(int number);
void drawCO2(int number);

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


//####################################################################################################
// Setup
//####################################################################################################
void setup() {
  delay(5000);
  Serial.begin(115200);
  Serial.println("A");
  delay(5000);

  // Set all chip selects high to avoid bus contention during initialisation of each peripheral
  digitalWrite(22, HIGH); // Touch controller chip select (if used)
  digitalWrite(15, HIGH); // TFT screen chip select
  digitalWrite( 5, HIGH); // SD card chips select, must use GPIO 5 (ESP32 SS)

  Serial.println("B");
  delay(5000);

  tft.begin();

  Serial.println("C");
  delay(5000);

  if (!SD.begin(5, tft.getSPIinstance())) {
    Serial.println("Card Mount Failed");
    return;
  }
  Serial.println("D");
  delay(5000);
  uint8_t cardType = SD.cardType();
  Serial.println("E");
  delay(5000);
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  delay(5000);
  Serial.println("F");
  Serial.print("SD Card Type: ");
  
  delay(5000);
  Serial.println("G");
  if (cardType == CARD_MMC) {
    delay(5000);
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    delay(5000);
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    delay(5000);
    Serial.println("SDHC");
  } else {
    delay(5000);
    Serial.println("UNKNOWN");
  }

  delay(5000);
  Serial.println("H");
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  delay(5000);
    Serial.println("I");
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  delay(5000);
  Serial.println("J");

  delay(10000);
  Serial.println("K");
  tft.setRotation(1);  // portrait
  tft.fillScreen(0xFFFF);
  delay(5000);
  Serial.println("L");
  drawSdJpeg("/screen_background.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card
  delay(5000);
  Serial.println("M");

  Serial.println("initialisation done.");
}

//####################################################################################################
// Main loop
//####################################################################################################
void loop() {
  // The image is 300 x 300 pixels so we do some sums to position image in the middle of the screen!
  // Doing this by reading the image width and height from the jpeg info is left as an exercise!
  // int x = (tft.width()  - 300) / 2 - 1;
  // int y = (tft.height() - 300) / 2 - 1;
  drawFahrenheit(random(0, 100));
  drawCelcius(random(0, 100));
  drawHumidity(random(0, 100));
  drawCO2(random(0, 1000));

  delay(3000);
  Serial.println("loop done"); 
}

//####################################################################################################
// Draw number images
//####################################################################################################

void drawFahrenheit(int number) {
  if(number > 0 && number < 100) {
    //parse number into individual characters
    int firstDigit = number / 10;
    int secondDigit = number % 10;

    //make string of the number
    String firstDigitString = "/"+String(firstDigit)+".jpg";
    String secondDigitString = "/"+String(secondDigit)+".jpg";
    drawSdJpeg(firstDigitString.c_str(), 60, 65);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg(secondDigitString.c_str(), 85, 65);     // This draws a jpeg pulled off the SD Card

  } else {
    //display error
    drawSdJpeg("/0.jpg", 85, 65);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg("/0.jpg", 60, 65);     // This draws a jpeg pulled off the SD Card
  }
}

void drawCelcius(int number) {
  if(number > 0 && number < 100) {
    //parse number into individual characters
    int firstDigit = number / 10;
    int secondDigit = number % 10;

    //make string of the number
    String firstDigitString = "/"+String(firstDigit)+".jpg";
    String secondDigitString = "/"+String(secondDigit)+".jpg";
    drawSdJpeg(firstDigitString.c_str(), 175, 65);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg(secondDigitString.c_str(), 200, 65);     // This draws a jpeg pulled off the SD Card

  } else {
    //display error zeros
    drawSdJpeg("/0.jpg", 175, 65);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg("/0.jpg", 200, 65);     // This draws a jpeg pulled off the SD Card
  }
}

void drawHumidity(int number) {
  if(number > 0 && number < 100) {
    //parse number into individual characters
    int firstDigit = number / 10;
    int secondDigit = number % 10;

    //make string of the number
    String firstDigitString = "/"+String(firstDigit)+".jpg";
    String secondDigitString = "/"+String(secondDigit)+".jpg";
    drawSdJpeg(firstDigitString.c_str(), 50, 175);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg(secondDigitString.c_str(), 75, 175);     // This draws a jpeg pulled off the SD Card

  } else {
    //display error zeros
    drawSdJpeg("/0.jpg", 175, 65);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg("/0.jpg", 200, 65);     // This draws a jpeg pulled off the SD Card
  }
}

void drawCO2(int number) {
  if(number > 0 && number < 1000) {
    //parse number into individual characters
    int firstDigit = number / 100;
    int secondDigit = number % 100 / 10;
    int thirdDigit = number % 10;

    //make string of the number
    String firstDigitString = "/"+String(firstDigit)+".jpg";
    String secondDigitString = "/"+String(secondDigit)+".jpg";
    String thirdDigitString = "/"+String(thirdDigit)+".jpg";
    drawSdJpeg(firstDigitString.c_str(), 175, 175);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg(secondDigitString.c_str(), 200, 175);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg(thirdDigitString.c_str(), 225, 175);     // This draws a jpeg pulled off the SD Card

  } else {
    //display error zeros
    drawSdJpeg("/0.jpg", 175, 175);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg("/0.jpg", 200, 175);     // This draws a jpeg pulled off the SD Card
    drawSdJpeg("/0.jpg", 225, 175);     // This draws a jpeg pulled off the SD Card
  }
}

//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(const char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Use one of the following methods to initialise the decoder:
  bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //bool decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  tft.setSwapBytes(swapBytes);

  showTime(millis() - drawTime); // These lines are for sketch testing only
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
// JpegDec.decodeFile(...) or JpegDec.decodeArray(...) must be called before this info is available!
void jpegInfo() {

  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}

//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16-bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
  //tft.setCursor(0, 0);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.print(F(" JPEG drawn in "));
  //tft.print(msTime);
  //tft.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}


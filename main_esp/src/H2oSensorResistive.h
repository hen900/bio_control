#include <Wire.h>
#include <Arduino.h>
#include <Wire.h>

#define THRESHOLD 100
#define MAX_LEVEL 450 //max analog reading - experimentally determined

class H2oSensorResistive {
public:
    int last_value = 0;

    H2oSensorResistive() {}

    void init(int dataPin, int powerPin) {
        Serial.print("Initializing water level sensor");
        this->dataPin = dataPin;
        this->powerPin = powerPin;
        pinMode(powerPin, OUTPUT);
        pinMode(dataPin, INPUT);
        delay(1000);
    }

    int read() {
        int new_level = 0;

        //Serial.print("Getting water level");
        digitalWrite(powerPin, HIGH);
        delay(1000);
        new_level = analogRead(dataPin);
        digitalWrite(powerPin, LOW);
        // Serial.print("Raw analog reading: ");
        // Serial.print(new_level);
       
        //calculate analog reading as an integer percentage
        this->last_value = (new_level/MAX_LEVEL)*100;
        return this->last_value;
    }  

private:
    int powerPin;
    int dataPin;
};  
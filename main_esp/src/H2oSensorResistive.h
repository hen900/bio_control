#include <Wire.h>
#include <Arduino.h>
#include <Wire.h>

#define THRESHOLD 100
#define MIN_LEVEL 1000 //min analog reading experimentally determined
#define MAX_LEVEL 3000 //max analog reading experimentally determined

class H2oSensorResistive {
public:
    double last_value = 0;

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
        double new_level = 0;

        //Serial.print("Getting water level");
        digitalWrite(powerPin, HIGH);
        delay(1000);
        new_level = analogRead(dataPin);
        digitalWrite(powerPin, LOW);
        Serial.print("\nRaw analog reading: ");
        Serial.println(new_level);

        //prevent returning negative values
        new_level = new_level - MIN_LEVEL;
        if (new_level < 0) {
            Serial.print("\nWater level too low");
            return 0;
        }
       
        //calculate analog reading as a double percentage
        this->last_value = (new_level / (MAX_LEVEL - MIN_LEVEL)) * 100;
        // return this->last_value rounded to the nearest 5
        return (int)(this->last_value - ((int)this->last_value % 5));
    } 

private:
    int powerPin;
    int dataPin;
};  
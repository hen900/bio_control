#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
//todo clean up and remove junk functions
class BioSensor {
public:
    BioSensor() {
        Wire.begin();
    }

    void init() {
        //Serial.print("\nIn bio Sensor init");
        SensirionI2CScd4x testScd4x;
        this->scd4x = testScd4x;
        this->scd4x.begin(Wire);
        uint16_t error;
        char errorMessage[256];

        //todo also figure out if this block is necessary. maybe start and stop only during readings?
        error = this->scd4x.stopPeriodicMeasurement();
        if (error) {
            handleErrorMessage("stopPeriodicMeasurement", error, errorMessage);
        }

        //todo figure out if this block is necessary
        uint16_t serial0;
        uint16_t serial1;
        uint16_t serial2;
        error = this->scd4x.getSerialNumber(serial0, serial1, serial2);
        if (error) {
            handleErrorMessage("getSerialNumber", error, errorMessage);
        } else {
            printSerialNumber(serial0, serial1, serial2);
        }

        error = this->scd4x.startPeriodicMeasurement();
        if (error) {
            handleErrorMessage("startPeriodicMeasurement", error, errorMessage);
        }
    }

    void read() {
        //todo: explore turning on scd4x.startPeriodicMeasurement() at the beginning of the function and stopping it at the end
        //Serial.print("Reading measurement");
        uint16_t error;
        char errorMessage[256];
        delay(100);

        uint16_t co2 = 0;
        float temperature = 0.0f;
        float humidity = 0.0f;
        bool isDataReady = false;
        Serial.print("\nA");
        error = this->scd4x.getDataReadyFlag(isDataReady);
        if (error) {
            handleErrorMessage("getDataReadyFlag", error, errorMessage);
            return;
        }
        if (!isDataReady) {
            Serial.print("\nB");
            return;
        }

        Serial.print("\nC");
        error = this->scd4x.readMeasurement(co2, temperature, humidity);
        if (error) {
            handleErrorMessage("readMeasurement", error, errorMessage);
        
        } else {
            Serial.print("\nD");
            this->co2Value = co2;
            this->temperatureValue = temperature;
            this->humidityValue = humidity;

            //Test code todo delete
            Serial.print("\n\nPrinting measurement");
            Serial.print("\nCO2 Value:"); 
            Serial.print(co2);
            Serial.print("\t\tTemp Value: "); 
            Serial.print(temperature);
            Serial.print("\tHumidity Value: "); 
            Serial.print(humidity);
            Serial.print("\n");
        }
    }

    uint16_t getCO2() {
        return co2Value;
    }

    float getTemperature() {
        return temperatureValue;
    }

    float getHumidity() {
        return humidityValue;
    }

    void handleErrorMessage(const char* operation, uint16_t error, char* errorMessage) {
        Serial.print("\nSensor read error trying to execute ");
        Serial.print(operation);
        Serial.print("(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
        Serial.print("Serial: 0x");
        printUint16Hex(serial0);
        printUint16Hex(serial1);
        printUint16Hex(serial2);
        Serial.println();
    }

    void printUint16Hex(uint16_t value) {
        Serial.print(value < 4096 ? "0" : "");
        Serial.print(value < 256 ? "0" : "");
        Serial.print(value < 16 ? "0" : "");
        Serial.print(value, HEX);
    }

private:
    SensirionI2CScd4x scd4x;
    uint16_t co2Value;
    float temperatureValue;
    float humidityValue;
};

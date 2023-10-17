#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

class BioSensor {
public:
    BioSensor() {
        Wire.begin();
    }

    void init() {
        scd4x.begin(Wire);

        uint16_t error;
        char errorMessage[256];

        error = scd4x.stopPeriodicMeasurement();
        if (error) {
            handleErrorMessage("stopPeriodicMeasurement", error, errorMessage);
        }

        uint16_t serial0;
        uint16_t serial1;
        uint16_t serial2;
        error = scd4x.getSerialNumber(serial0, serial1, serial2);
        if (error) {
            handleErrorMessage("getSerialNumber", error, errorMessage);
        } else {
            printSerialNumber(serial0, serial1, serial2);
        }

        error = scd4x.startPeriodicMeasurement();
        if (error) {
            handleErrorMessage("startPeriodicMeasurement", error, errorMessage);
        }
    }

    void read() {
        uint16_t error;
        char errorMessage[256];

        delay(100);

        uint16_t co2 = 0;
        float temperature = 0.0f;
        float humidity = 0.0f;
        bool isDataReady = false;
        error = scd4x.getDataReadyFlag(isDataReady);
        if (error) {
            handleErrorMessage("getDataReadyFlag", error, errorMessage);
            return;
        }
        if (!isDataReady) {
            return;
        }
        error = scd4x.readMeasurement(co2, temperature, humidity);
        if (error) {
            handleErrorMessage("readMeasurement", error, errorMessage);
        
        } else {
            co2Value = co2;
            temperatureValue = temperature;
            humidityValue = humidity;
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

private:
    SensirionI2CScd4x scd4x;
    uint16_t co2Value;
    float temperatureValue;
    float humidityValue;

    void handleErrorMessage(const char* operation, uint16_t error, char* errorMessage) {
        Serial.print("Error trying to execute ");
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
};

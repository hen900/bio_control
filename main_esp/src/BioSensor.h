#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

class BioSensor {
private:
    SensirionI2CScd4x scd4x;
    uint16_t co2Value;
    float temperatureValue;
    float humidityValue;
    const uint16_t CO2_OFFSET = 0;
    const float TEMP_OFFSET = 10.0f;

public:
    BioSensor() {
        Serial.print("\nIn bio Sensor constructor");
        Wire.begin(); // Initialize I2C communication
    }

    void init() {
        Serial.print("\nIn bio Sensor init");
        scd4x.begin(Wire); // Initialize the sensor interface

        // //Enable automatic self calibration
        // uint16_t error = scd4x.setAutomaticSelfCalibration(true);
        // if (error) {
        //     handleErrorMessage("setAutomaticSelfCalibration", error);
        // }

        //Start periodic measurement
        uint16_t error = scd4x.startPeriodicMeasurement();
        if (error) {
            handleErrorMessage("startPeriodicMeasurement", error);
        }
    }

    void read() {
        bool data_ready = true;
        uint16_t error = scd4x.getDataReadyFlag(data_ready);
        int start_time = millis();

        Serial.print("\nChecking data ready flag");
        //Wait until data is ready
        while (data_ready != 0) {
            //Timeout after 5 seconds
            if (millis() - start_time > 5000) {
                Serial.print("\nSensor read timeout");
                return;
            }

            error = scd4x.getDataReadyFlag(data_ready);
            if (error) {
                //handleErrorMessage("dataReady", error);
                Serial.print(".");
                delay(1000);
                //scd4x.reinit(); //use if necessary
            } else {
                break;
            }
        }
        
        Serial.print("\nReading measurement");
        uint16_t co2 = 0;
        float temperature = 0.0f;
        float humidity = 0.0f;

        error = scd4x.readMeasurement(co2, temperature, humidity);
        if (error) {
            handleErrorMessage("readMeasurement", error);
            delay(1000);
            scd4x.reinit();
        } else {
            co2Value = co2;
            temperatureValue = temperature;
            humidityValue = humidity;

            // Serial.print("\n\nPrinting raw measured values");
            // Serial.print("\nCO2 Value:");
            // Serial.print(co2);
            // Serial.print("\t\tTemp Value: ");
            // Serial.print(temperature);
            // Serial.print("\tHumidity Value: ");
            // Serial.print(humidity);
            // Serial.print("\n");
        }
    }

    uint16_t getCO2() {
        return co2Value - CO2_OFFSET;
    }

    float getTemperature() {
        return temperatureValue - TEMP_OFFSET;
    }

    float getHumidity() {
        return humidityValue;
    }

    void handleErrorMessage(const char* operation, uint16_t error) {
        Serial.print("\nSensor read error trying to execute ");
        Serial.print(operation);
        Serial.print("(): Error code ");
        Serial.println(error);
    }

    // int calibrate(int targetCo2) {
    //     uint16_t target_Co2 = targetCo2;
    //     uint16_t offset = 400;

    //     //Calibrate sensor
    //     //scd4x.stopPeriodicMeasurement();
    //     delay(1000); //necessary delay

    //     //Enable automatic self calibration
    //     uint16_t error = scd4x.performForcedRecalibration(targetCo2, offset);
    //     delay(1000); //necessary delay
    //     if (error) {
    //         handleErrorMessage("setAutomaticSelfCalibration", error);
    //         return 0; 
    //     }

    //     //Start periodic measurement
    //     //error = scd4x.startPeriodicMeasurement();
    //     return 1;
    // }
};
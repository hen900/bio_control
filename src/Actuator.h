#include <Arduino.h>
#include <Wire.h>

class Actuator {
public:
	Actuator() {}
	Actuator(int pin) : pin(pin), status("false"), actuatorName("") {}

	void init(int pin) {
		this->pin = pin;
		this->status = "false";
		this->actuatorName = "";
		pinMode(pin, OUTPUT);
	}

	void toggle() {
		Serial.print("\nToggling Pin");
		Serial.print(pin);

		if (status == "false") {
			this->on();
		} else {
			this->off();
		}
	}

	String getStatus() {
		return this->status;
	}

	void setStatus(String newStatus) {
		String oldStatus = this->status;
		this->status = newStatus;

		if(oldStatus != newStatus) {
			if(newStatus == "true") {
				this->on();
			} else if(newStatus == "false") {
				this->off(); 
			} else {
				Serial.print("\nInvalid new status: "); Serial.print(newStatus); 
			}
		}
	}

	bool getPin() {
		return this->pin;
	}

	void setPin(bool level) {
		digitalWrite(this->pin, level);
	}

	void on() {
		Serial.print("\nTurning on actuator on pin"); Serial.print(this->pin);
		digitalWrite(this->pin, HIGH);
		this->status = "true";
	}

	void off() {
		Serial.print("\nTurning off actuator on pin"); Serial.print(this->pin);
		digitalWrite(this->pin, LOW);
		this->status = "false";
	}

private:
	int pin;
	String status;
	String actuatorName;
};
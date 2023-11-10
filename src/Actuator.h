#include <Arduino.h>
#include <Wire.h>

class Actuator {
public:
	Actuator() {}
	Actuator(int pin) : pin(pin), status(false) {}

	void init(int pin) {
		pinMode(pin, OUTPUT);
		off();
		Serial.print("we actually made it to the init function");
	}

	void toggle() {
		Serial.print("\nToggling Pin");
		Serial.print(pin);

		if (status) {
			this->off();
		} else {
			this->on();
		}
	}

	bool getStatus() {
		return status;
	}

	void setStatus(bool newStatus) {
		status = newStatus;
		if(newStatus) {
			this->on();
		} else {
			this->off(); 
		}
	}

	bool getPin() {
		return pin;
	}

	void setPin(bool level) {
		digitalWrite(pin, level);
	}

private:
	int pin;
	bool status;

	void on() {
		digitalWrite(pin, HIGH);
		status = true;
	}

	void off() {
		digitalWrite(pin, LOW);
		status = false;
	}
};
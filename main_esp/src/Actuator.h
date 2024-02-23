#include <Arduino.h>
#include <Wire.h>

class Actuator {
public:
	Actuator() {}
	Actuator(int pin) : pin(pin), status(false), actuatorName("") {}

	void init(int pin) {
		this->pin = pin;
		this->status = false;
		this->actuatorName = "";
		pinMode(pin, OUTPUT);
	}

	void toggle() {
		Serial.print("\nToggling Pin");
		Serial.print(pin);

		if (status == false) {
			this->on();
		} else {
			this->off();
		}
	}

	boolean getStatus() {
		return this->status;
	}

	void setStatus(String newStatus) {
		boolean newStatusBool;

		if(newStatus == "true") {
			this->on();
			this->status = true;

		} else if (newStatus == "false") {
			this->off();
			this->status = false;
		} else {
			Serial.print("\nInvalid new status: "); Serial.print(newStatus);
		}

		Serial.print("Status set to ");
		Serial.print(this->status);
		Serial.print(" on pin ");
		Serial.print(this->pin);
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
		this->status = true;
	}

	void off() {
		Serial.print("\nTurning off actuator on pin"); Serial.print(this->pin);
		digitalWrite(this->pin, LOW);
		this->status = false;
	}

private:
	int pin;
	boolean status;
	String actuatorName;
};

//TEST CODE
// Serial.print("\nOld Status");
		// Serial.print(oldStatus);
		// Serial.print("\nNew Status");
		// Serial.print(newStatus);
		// Serial.print("\nThis Status");
		// Serial.println(this->status);
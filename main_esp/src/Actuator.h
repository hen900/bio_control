#include <Arduino.h>
#include <Wire.h>

class Actuator {
public:
	Actuator() {}
	Actuator(int pin) : pin(pin), status(false), actuatorName("") {}
	int override = 2; // 0 = override to false, 1 = override to true, 2 = do not override

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

	void setStatus(int newStatus) {
		boolean newStatusBool;

		if(newStatus == 1) {
			this->on();
			this->status = true;

		} else if (newStatus == 0) {
			this->off();
			this->status = false;
		} else {
			Serial.print("\nInvalid new status: "); Serial.print(newStatus);
		}

		// Serial.print("Status set to ");
		// Serial.print(this->status);
		// Serial.print(" on pin ");
		// Serial.print(this->pin);
	}

	bool getPin() {
		return this->pin;
	}

	void setPin(bool level) {
		digitalWrite(this->pin, level);
	}

	void on() {
		if (this->status == true) { return; }
		Serial.print("Turning on actuator on pin "); Serial.println(this->pin);
		digitalWrite(this->pin, HIGH);
		this->status = true;
	}

	void off() {
		if (this->status == false) { return; }
		Serial.print("Turning off actuator on pin "); Serial.println(this->pin);
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
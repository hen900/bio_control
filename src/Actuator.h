#include <Arduino.h>
#include <Wire.h>

class Actuator {
public:
  Actuator() {}
  Actuator(int pin) : pin(pin), status(false) {}

  void init(int pin) {
    pinMode(pin, OUTPUT);
    off();
  }

  void on() {
    digitalWrite(pin, HIGH);
    status = true;
  }

  void off() {
    digitalWrite(pin, LOW);
    status = false;
  }

  void toggle() {
    if (status) {
      off();
    } else {
      on();
    }
  }

  bool getStatus() {
    return status;
  }

private:
  int pin;
  bool status;
};
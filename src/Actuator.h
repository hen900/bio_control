#include <Arduino.h>
#include <Wire.h>

class Actuator {
public:
  Actuator() {}
  Actuator(int pin) : pin(pin), state(false) {}

  void init(int pin) {
    pinMode(pin, OUTPUT);
    off();
  }

  void on() {
    digitalWrite(pin, HIGH);
    state = true;
  }

  void off() {
    digitalWrite(pin, LOW);
    state = false;
  }

  void toggle() {
    if (state) {
      off();
    } else {
      on();
    }
  }

  bool getState() {
    return state;
  }

private:
  int pin;
  bool state;
};
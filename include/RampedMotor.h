#pragma once
#include <Arduino.h>

// PWM slew limiter shared by both motor channels. No blocking delays.
class RampedMotor {
public:
  RampedMotor(uint8_t pwm, uint8_t dir) : pwmPin(pwm), dirPin(dir) {}
  void begin();
  void run(uint8_t direction, uint8_t speed);
  void stop();
  void update(unsigned long now);

private:
  uint8_t pwmPin, dirPin, direction = LOW, requestedDirection = LOW;
  uint8_t target = 0;
  float pwm = 0;
  unsigned long lastUpdate = 0, zeroSince = 0;
  bool pausing = false;
};

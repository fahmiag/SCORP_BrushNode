#pragma once
#include "RampedMotor.h"
#include "Config.h"
class AxisMotor {
public:
  void begin();
  void run(bool positive);
  void stop();
  void update(unsigned long now);

private:
  RampedMotor motor{Config::axisPwm, Config::axisDir};
};
extern AxisMotor axisMotor;

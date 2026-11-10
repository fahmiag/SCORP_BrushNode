#pragma once
#include "RampedMotor.h"
#include "Config.h"
class BrushMotor {
public:
  void begin();
  void run(bool positive);
  void stop();
  void update(unsigned long now);

private:
  RampedMotor motor{Config::motorPwm, Config::motorDir};
};
extern BrushMotor brushMotor;

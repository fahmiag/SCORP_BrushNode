#include "brushMotor.h"
BrushMotor brushMotor;

void BrushMotor::begin()
{
  motor.begin();
}

void BrushMotor::run(bool positive)
{
  motor.run(positive ? Config::forwardLevel : !Config::forwardLevel, Config::brushSpeed);
}

void BrushMotor::stop()
{
  motor.stop();
}

void BrushMotor::update(unsigned long now)
{
  motor.update(now);
}

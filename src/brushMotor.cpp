#include "brushMotor.h"
#include "debug.h"
BrushMotor brushMotor;

void BrushMotor::begin()
{
    motor.begin();
}

void BrushMotor::run(bool positive)
{
    motor.run(positive ? Config::forwardLevel : !Config::forwardLevel, Config::brushSpeed);
    Debug::motorAction(positive ? "BRUSH MOVE FORWARD" : "BRUSH MOVE REVERSE");
}

void BrushMotor::stop()
{
    motor.stop();
    Debug::motorAction("BRUSH STOP");
}

void BrushMotor::update(unsigned long now)
{
    motor.update(now);
}

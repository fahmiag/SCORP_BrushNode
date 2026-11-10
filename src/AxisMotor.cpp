#include "AxisMotor.h"
AxisMotor axisMotor;

void AxisMotor::begin()
{
    motor.begin();
}

void AxisMotor::run(bool positive)
{
    motor.run(positive ? Config::upLevel : !Config::upLevel, Config::axisSpeed);
}

void AxisMotor::stop()
{
    motor.stop();
}

void AxisMotor::update(unsigned long now)
{
    motor.update(now);
    
}

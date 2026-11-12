#include "AxisMotor.h"
#include "debug.h"
AxisMotor axisMotor;

void AxisMotor::begin()
{
    motor.begin();
}

void AxisMotor::run(bool positive)
{
    motor.run(positive ? Config::upLevel : !Config::upLevel, Config::axisSpeed);
    Debug::motorAction(positive ? "MOVE UP" : "MOVE DOWN");
}

void AxisMotor::stop()
{
    motor.stop();
    Debug::motorAction("MOVE STOP");
}

void AxisMotor::update(unsigned long now)
{
    motor.update(now);
    
}

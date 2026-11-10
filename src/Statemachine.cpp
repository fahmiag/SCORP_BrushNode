#include "Statemachine.h"

#include "AxisMotor.h"
#include "brushMotor.h"

StateMachine stateMachine;

StateMachine::Limit::Limit(uint8_t pin) : pin(pin)
{
}

void StateMachine::Limit::update(unsigned long now)
{
    if (digitalRead(pin) == LOW)
    {
        active = true;
        lastLow = now;
    }
    else if (now - lastLow >= 20)
    {
        active = false;
    }
}

void StateMachine::begin()
{
    axisMotor.begin();
    brushMotor.begin();
    pinMode(Config::topLimit, INPUT_PULLUP);
    pinMode(Config::bottomLimit, INPUT_PULLUP);
}

void StateMachine::update(unsigned long now)
{
    top.update(now);
    bottom.update(now);
    updateAxis(now);
}

void StateMachine::stopAxis()
{
    axisMotor.stop();
    axis = Axis::Idle;
}

void StateMachine::stopAll()
{
    brushMotor.stop();
    stopAxis();
}

void StateMachine::trip(const char *reason)
{
    stopAll();
    fault = true;
    Serial1.print("FAULT ");
    Serial1.println(reason);
}

void StateMachine::startLeg(Axis next, unsigned long now)
{
    axisMotor.stop();
    stateSince = now;
    axis = next;
    axisMotor.run(next == Axis::Up);
}

void StateMachine::updateAxis(unsigned long now)
{
    if (bothLimits())
    {
        if (!fault)
        {
            trip("BOTH_LIMITS");
        }
        return;
    }

    switch (axis)
    {
        case Axis::Up:
            if (top.active)
            {
                axisMotor.stop();
                axis = Axis::Wait;
                stateSince = now;
            }
            else if (now - stateSince >= Config::travelTimeoutMs)
            {
                trip("UP_TIMEOUT");
            }
            break;

        case Axis::Wait:
            if (now - stateSince >= Config::topWaitMs)
            {
                if (bottom.active)
                {
                    stopAxis();
                }
                else
                {
                    startLeg(Axis::Down, now);
                }
            }
            break;

        case Axis::Down:
            if (bottom.active)
            {
                stopAxis();
                Serial1.println("EVENT CYCLE_DONE");
            }
            else if (now - stateSince >= Config::travelTimeoutMs)
            {
                trip("DOWN_TIMEOUT");
            }
            break;

        case Axis::Idle:
            break;
    }
}

bool StateMachine::bothLimits() const
{
    return top.active && bottom.active;
}

bool StateMachine::topActive() const
{
    return top.active;
}

bool StateMachine::bottomActive() const
{
    return bottom.active;
}

Axis StateMachine::axisState() const
{
    return axis;
}

bool StateMachine::hasFault() const
{
    return fault;
}

void StateMachine::clearFault()
{
    fault = false;
}

bool StateMachine::startCycle(unsigned long now)
{
    if (axis != Axis::Idle || fault)
    {
        return false;
    }

    if (top.active)
    {
        axis = Axis::Wait;
        stateSince = now;
    }
    else
    {
        startLeg(Axis::Up, now);
    }
    return true;
}

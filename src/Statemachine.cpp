#include "Statemachine.h"
#include "Config.h"
#include "debug.h"
#include "AxisMotor.h"
#include "brushMotor.h"
StateMachine stateMachine;

StateMachine::Limit::Limit(uint8_t pin, uint8_t activeLevel)
    : pin(pin), activeLevel(activeLevel)
{
}

void StateMachine::Limit::update(unsigned long now)
{
    if (digitalRead(pin) == activeLevel)
    {
        active = true;
        lastActive = now;
    }
    else if (now - lastActive >= 20)
    {
        active = false;
    }
}
void StateMachine::stopAxis()
{
    axisMotor.stop();
    axis = Axis::Idle;
    resumeState = Axis::Up;
    pausedElapsed = 0;
}

void StateMachine::pauseAxis(unsigned long now)
{
    if (axis == Axis::Paused || axis == Axis::Idle)
    {
        return;
    }

    resumeState = axis;
    pausedElapsed = now - stateSince;
    axisMotor.stop();
    axis = Axis::Paused;
}

bool StateMachine::setDirection(Axis next)
{
    if (hasFault() || (axis != Axis::Idle && axis != Axis::Paused) ||
        (next != Axis::Up && next != Axis::Down))
    {
        return false;
    }

    resumeState = next;
    pausedElapsed = 0;
    return true;
}

Axis StateMachine::direction() const
{
    if (axis == Axis::Up || axis == Axis::Down)
    {
        return axis;
    }
    return resumeState == Axis::Down ? Axis::Down : Axis::Up;
}

void StateMachine::stopAll()
{
    brushMotor.stop();
    stopAxis();
}

void StateMachine::trip(Fault reason)
{
    stopAll();
    if (hasFault()) return; // Preserve the first cause until CLEAR succeeds.
    fault = reason;
    Debug::port().print("FAULT ");
    Debug::port().println(faultReason());
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
    if (top.active && bottom.active)
    {
        if (!hasFault())
        {
            trip(Fault::BothLimits);
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
                trip(Fault::UpTimeout);
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
                Debug::port().println("EVENT CYCLE_DONE");
            }
            else if (now - stateSince >= Config::travelTimeoutMs)
            {
                trip(Fault::DownTimeout);
            }
            break;

        case Axis::Idle:
        case Axis::Paused:
            break;
    }
}

void StateMachine::begin()
{
    axisMotor.begin();
    brushMotor.begin();
    pinMode(Config::topLimit, INPUT_PULLUP);
    pinMode(Config::bottomLimit, INPUT_PULLUP);
    pinMode(Config::moveUpButton, INPUT_PULLUP);
    pinMode(Config::moveDownButton, INPUT_PULLUP);
    pinMode(Config::moveStopButton, INPUT_PULLUP);
    // A button held at startup must be released before it can start motion.
    const unsigned long now = millis();
    top.update(now);
    bottom.update(now);
    upButton.update(now);
    downButton.update(now);
    stopButton.update(now);
}
void StateMachine::updateButtons(unsigned long now)
{
    const bool wasUp = upButton.active;
    const bool wasDown = downButton.active;
    upButton.update(now);
    downButton.update(now);
    stopButton.update(now);

    // Stop wins, including conflicting direction buttons. No queued starts.
    if (stopButton.active || (upButton.active && downButton.active))
    {
        pauseAxis(now);
        return;
    }
    if (hasFault() || (axis != Axis::Idle && axis != Axis::Paused))
    {
        return;
    }

    const bool pressedUp = upButton.active && !wasUp;
    const bool pressedDown = downButton.active && !wasDown;
    if (!pressedUp && !pressedDown)
    {
        return;
    }
    const Axis next = pressedUp ? Axis::Up : Axis::Down;
    // Resume the same leg without granting a fresh travel timeout.
    if (axis == Axis::Idle || resumeState != next)
    {
        if (!setDirection(next)) return;
    }
    startCycle(now);
}

void StateMachine::update(unsigned long now)
{
    top.update(now);
    bottom.update(now);
    updateAxis(now);
    updateButtons(now);
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
    return fault != Fault::None;
}

void StateMachine::clearFault()
{
    fault = Fault::None;
}

const char *StateMachine::faultReason() const
{
    switch (fault)
    {
        case Fault::BothLimits: return "BOTH_LIMITS";
        case Fault::UpTimeout: return "UP_TIMEOUT";
        case Fault::DownTimeout: return "DOWN_TIMEOUT";
        case Fault::CommunicationTimeout: return "COMM_TIMEOUT";
        default: return "NONE";
    }
}

bool StateMachine::startCycle(unsigned long now)
{
    // Physical stop also inhibits serial starts while held or debouncing.
    if (stopButton.active || digitalRead(Config::moveStopButton) == LOW ||
        (upButton.active && downButton.active))
    {
        return false;
    }
    if ((axis != Axis::Idle && axis != Axis::Paused) || hasFault() || bothLimits())
    {
        return false;
    }

    const Axis next = resumeState;
    if (next == Axis::Down && bottom.active)
    {
        return false;
    }

    if (axis == Axis::Paused && next == Axis::Wait)
    {
        axis = Axis::Wait;
        stateSince = now - pausedElapsed;
    }
    else if (next == Axis::Up && top.active)
    {
        axis = Axis::Wait;
        stateSince = now;
    }
    else
    {
        startLeg(next, now);
        stateSince = now - pausedElapsed;
    }
    pausedElapsed = 0;
    return true;
}

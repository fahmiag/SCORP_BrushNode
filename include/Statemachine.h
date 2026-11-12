#pragma once

#include <Arduino.h>
#include "Config.h"

enum class Axis
{
    Idle,
    Up,
    Wait,
    Down,
    Paused
};

class StateMachine
{
public:
    void begin();
    void update(unsigned long now);
    void stopAxis();
    void pauseAxis(unsigned long now);
    bool setDirection(Axis direction);
    Axis direction() const;
    void stopAll();
    void trip(const char *reason);
    void clearFault();
    bool startCycle(unsigned long now);

    bool bothLimits() const;
    bool topActive() const;
    bool bottomActive() const;
    Axis axisState() const;
    bool hasFault() const;

private:
    // Stop immediately on switch closure; debounce release for 20 ms.
    struct Limit
    {
        explicit Limit(uint8_t pin);
        void update(unsigned long now);

        uint8_t pin;
        bool active = false;
        unsigned long lastLow = 0;
    };

    void startLeg(Axis next, unsigned long now);
    void updateAxis(unsigned long now);
    void updateButtons(unsigned long now);

    Axis axis = Axis::Idle;
    unsigned long stateSince = 0;
    bool fault = false;
    Axis resumeState = Axis::Up;
    unsigned long pausedElapsed = 0;
    Limit top{Config::topLimit};
    Limit bottom{Config::bottomLimit};
    Limit upButton{Config::moveUpButton};
    Limit downButton{Config::moveDownButton};
    Limit stopButton{Config::moveStopButton};
};

extern StateMachine stateMachine;

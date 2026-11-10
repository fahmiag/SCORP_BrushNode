#pragma once

#include <Arduino.h>
#include "Config.h"

enum class Axis
{
    Idle,
    Up,
    Wait,
    Down
};

class StateMachine
{
public:
    void begin();
    void update(unsigned long now);
    void stopAxis();
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

    Axis axis = Axis::Idle;
    unsigned long stateSince = 0;
    bool fault = false;
    Limit top{Config::topLimit};
    Limit bottom{Config::bottomLimit};
};

extern StateMachine stateMachine;

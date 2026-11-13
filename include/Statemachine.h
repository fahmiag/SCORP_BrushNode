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

enum class Fault
{
    None,
    BothLimits,
    UpTimeout,
    DownTimeout,
    CommunicationTimeout
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
    void trip(Fault reason);
    void clearFault();
    bool startCycle(unsigned long now);

    bool bothLimits() const;
    bool topActive() const;
    bool bottomActive() const;
    Axis axisState() const;
    bool hasFault() const;
    const char *faultReason() const;

private:
    // Activate immediately; debounce return to the inactive level for 20 ms.
    struct Limit
    {
        explicit Limit(uint8_t pin, uint8_t activeLevel = LOW);
        void update(unsigned long now);

        uint8_t pin;
        uint8_t activeLevel;
        bool active = false;
        unsigned long lastActive = 0;
    };

    void startLeg(Axis next, unsigned long now);
    void updateAxis(unsigned long now);
    void updateButtons(unsigned long now);

    Axis axis = Axis::Idle;
    unsigned long stateSince = 0;
    Fault fault = Fault::None;
    Axis resumeState = Axis::Up;
    unsigned long pausedElapsed = 0;
    // NC limits open when actuated; INPUT_PULLUP then reads HIGH.
    Limit top{Config::topLimit, HIGH};
    Limit bottom{Config::bottomLimit, HIGH};
    Limit upButton{Config::moveUpButton};
    Limit downButton{Config::moveDownButton};
    Limit stopButton{Config::moveStopButton};
};

extern StateMachine stateMachine;

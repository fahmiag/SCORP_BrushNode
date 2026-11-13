#include <Arduino.h>
#include "Watchdog.h"

#include "App.h"
#include "Config.h"
#include "debug.h"

namespace
{
    void holdMotorInputsLow()
    {
        // Disable both channels before configuring either direction input.
        digitalWrite(Config::motorPwm, LOW);
        pinMode(Config::motorPwm, OUTPUT);
        digitalWrite(Config::axisPwm, LOW);
        pinMode(Config::axisPwm, OUTPUT);

        digitalWrite(Config::motorDir, LOW);
        pinMode(Config::motorDir, OUTPUT);
        digitalWrite(Config::axisDir, LOW);
        pinMode(Config::axisDir, OUTPUT);
    }
}

void setup()
{
    // External pull-downs must hold these inputs low before setup runs.
    holdMotorInputsLow();

    if (!Watchdog::begin(Config::watchdogTimeoutMs))
    {
        // Never accept motor commands without watchdog protection.
        Debug::begin();
        Debug::port().println("FAULT WATCHDOG_INIT");
        while (true)
        {
            delay(100);
        }
    }

    App::begin();
}

void loop()
{
    App::update();

    // Feed only after limits, motor updates and command handling complete.
    // A stalled loop must reset even if interrupts are still running.
    Watchdog::refresh();
}

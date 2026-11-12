#include "App.h"
#include "Config.h"
#include "debug.h"
#include "Statemachine.h"
#include "AxisMotor.h"
#include "brushMotor.h"
#include <string.h>

namespace
{
    unsigned long lastCommand = 0;
    bool connected = false;
    char line[48];
    size_t length = 0;
    bool overflow = false;

    bool isCommand(const char *s)
    {
        return strcmp(line, s) == 0;
    }

    void handleCommand(unsigned long now)
    {
        bool known =
                isCommand("PING") ||
                isCommand("STATUS") ||
                isCommand("STOP") ||
                isCommand("CLEAR") ||
                isCommand("MOTOR OFF") ||
                isCommand("MOTOR FORWARD") ||
                isCommand("MOTOR REVERSE") ||
                isCommand("AXIS START") ||
                isCommand("AXIS UP") ||
                isCommand("AXIS DOWN") ||
                isCommand("AXIS STOP");

        if (!known)
        {
            Debug::port().println("ERR COMMAND");
            return;
        }
        connected = true;
        lastCommand = now;

        if (isCommand("STOP"))
        {
            stateMachine.stopAll();
        }
        else if (isCommand("MOTOR OFF"))
        {
            brushMotor.stop();
        }
        else if (isCommand("AXIS STOP"))
        {
            stateMachine.pauseAxis(now);
        }
        else if (isCommand("CLEAR"))
        {
            stateMachine.stopAll();
            if (stateMachine.bothLimits())
            {
                Debug::port().println("ERR BOTH_LIMITS");
                return;
            }
            stateMachine.clearFault();
        }
        else if (isCommand("STATUS"))
        {
            const char *names[] = {"IDLE", "UP", "WAIT", "DOWN", "PAUSED"};
            Debug::port().print("STATUS AXIS=");
            Debug::port().print(names[static_cast<unsigned>(stateMachine.axisState())]);
            Debug::port().print(" DIR=");
            Debug::port().print(stateMachine.direction() == Axis::Up ? "UP" : "DOWN");
            Debug::port().print(" TOP=");
            Debug::port().print(stateMachine.topActive());
            Debug::port().print(" BOTTOM=");
            Debug::port().print(stateMachine.bottomActive());
            Debug::port().print(" FAULT=");
            Debug::port().println(stateMachine.hasFault());
            return;
        }
        else if (!isCommand("PING"))
        {
            if (stateMachine.hasFault())
            {
                Debug::port().println("ERR FAULT_CLEAR_REQUIRED");
                return;
            }
            if (isCommand("AXIS START"))
            {
                if (!stateMachine.startCycle(now))
                {
                    Debug::port().println("ERR AXIS_NOT_READY");
                    return;
                }
            }
            else if (isCommand("AXIS UP") || isCommand("AXIS DOWN"))
            {
                const Axis next = isCommand("AXIS UP") ? Axis::Up : Axis::Down;
                if (!stateMachine.setDirection(next))
                {
                    Debug::port().println("ERR AXIS_STOP_REQUIRED");
                    return;
                }
            }
            else
            {
                brushMotor.run(isCommand("MOTOR FORWARD"));
            }
        }
        Debug::port().println("OK");
    }
}

void App::begin()
{
    stateMachine.begin();
    Debug::begin();
}

void App::update()
{
    unsigned long now = millis();

    stateMachine.update(now);

    if (Debug::communicationTimeoutEnabled && connected &&
        now - lastCommand >= Config::communicationTimeoutMs)
    {
        connected = false;
        stateMachine.trip("COMM_TIMEOUT");
    }

    // State transitions can reset ramp timestamps; take a fresh time afterward.
    unsigned long motorNow = millis();
    axisMotor.update(motorNow);
    brushMotor.update(motorNow);

    // Bound parsing work to keep checking switches during continuous input.
    for (uint8_t n = 0; n < 32 && Debug::port().available(); ++n)
    {
        char c = static_cast<char>(Debug::port().read());

        if (c == '\r' || c == '\n') //end of line
        {
            if (overflow)
            {
                Debug::port().println("ERR LINE_TOO_LONG");
            }
            else if (length)  //length not 0
            {
                line[length] = '\0';   //set terminating null character

                handleCommand(millis());
            }
            length = 0;
            overflow = false;

        }
        else if (!overflow)
        {
            if (length < sizeof(line) - 1)
            {
                line[length++] = c;  //save into buffer and increment length
            }
            else overflow = true;
        }
    }
}

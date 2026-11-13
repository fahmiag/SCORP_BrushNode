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
                isCommand("BRUSH OFF") ||
                isCommand("BRUSH FORWARD") ||
                isCommand("BRUSH REVERSE") ||
                isCommand("MOVE START") ||
                isCommand("MOVE UP") ||
                isCommand("MOVE DOWN") ||
                isCommand("MOVE STOP");

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
        else if (isCommand("BRUSH OFF"))
        {
            brushMotor.stop();
        }
        else if (isCommand("MOVE STOP"))
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
            Debug::port().print("STATUS MOVE=");
            Debug::port().print(names[static_cast<unsigned>(stateMachine.axisState())]);
            Debug::port().print(" DIR=");
            Debug::port().print(stateMachine.direction() == Axis::Up ? "UP" : "DOWN");
            Debug::port().print(" TOP=");
            Debug::port().print(stateMachine.topActive());
            Debug::port().print(" BOTTOM=");
            Debug::port().print(stateMachine.bottomActive());
            Debug::port().print(" FAULT=");
            Debug::port().print(stateMachine.hasFault());
            Debug::port().print(" REASON=");
            Debug::port().print(stateMachine.faultReason());
            Debug::port().print(" MODE=");
            Debug::port().println(Debug::modeName);
            return;
        }
        else if (!isCommand("PING"))
        {
            if (stateMachine.hasFault())
            {
                Debug::port().println("ERR FAULT_CLEAR_REQUIRED");
                return;
            }
            if (isCommand("MOVE START"))
            {
                if (!stateMachine.startCycle(now))
                {
                    Debug::port().println("ERR MOVE_NOT_READY");
                    return;
                }
            }
            else if (isCommand("MOVE UP") || isCommand("MOVE DOWN"))
            {
                const Axis next = isCommand("MOVE UP") ? Axis::Up : Axis::Down;
                if (!stateMachine.setDirection(next))
                {
                    Debug::port().println("ERR MOVE_STOP_REQUIRED");
                    return;
                }
            }
            else
            {
                brushMotor.run(isCommand("BRUSH FORWARD"));
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
        stateMachine.trip(Fault::CommunicationTimeout);
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

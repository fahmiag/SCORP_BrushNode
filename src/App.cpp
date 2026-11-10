#include "App.h"
#include "Config.h"
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
            Serial1.println("ERR COMMAND");
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
                Serial1.println("ERR BOTH_LIMITS");
                return;
            }
            stateMachine.clearFault();
        }
        else if (isCommand("STATUS"))
        {
            const char *names[] = {"IDLE", "UP", "WAIT", "DOWN", "PAUSED"};
            Serial1.print("STATUS AXIS=");
            Serial1.print(names[static_cast<unsigned>(stateMachine.axisState())]);
            Serial1.print(" DIR=");
            Serial1.print(stateMachine.direction() == Axis::Up ? "UP" : "DOWN");
            Serial1.print(" TOP=");
            Serial1.print(stateMachine.topActive());
            Serial1.print(" BOTTOM=");
            Serial1.print(stateMachine.bottomActive());
            Serial1.print(" FAULT=");
            Serial1.println(stateMachine.hasFault());
            return;
        }
        else if (!isCommand("PING"))
        {
            if (stateMachine.hasFault())
            {
                Serial1.println("ERR FAULT_CLEAR_REQUIRED");
                return;
            }
            if (isCommand("AXIS START"))
            {
                if (!stateMachine.startCycle(now))
                {
                    Serial1.println("ERR AXIS_NOT_READY");
                    return;
                }
            }
            else if (isCommand("AXIS UP") || isCommand("AXIS DOWN"))
            {
                const Axis next = isCommand("AXIS UP") ? Axis::Up : Axis::Down;
                if (!stateMachine.setDirection(next))
                {
                    Serial1.println("ERR AXIS_STOP_REQUIRED");
                    return;
                }
            }
            else
            {
                brushMotor.run(isCommand("MOTOR FORWARD"));
            }
        }
        Serial1.println("OK");
    }
}

void App::begin()
{
    stateMachine.begin();
    Serial1.begin(Config::baud, SERIAL_8N1);
}

void App::update()
{
    unsigned long now = millis();

    stateMachine.update(now);

    if (connected && now - lastCommand >= Config::communicationTimeoutMs)
    {
        connected = false;
        stateMachine.trip("COMM_TIMEOUT");
    }

    // State transitions can reset ramp timestamps; take a fresh time afterward.
    unsigned long motorNow = millis();
    axisMotor.update(motorNow);
    brushMotor.update(motorNow);

    // Bound parsing work to keep checking switches during continuous input.
    for (uint8_t n = 0; n < 32 && Serial1.available(); ++n)
    {
        char c = static_cast<char>(Serial1.read());

        if (c == '\r' || c == '\n') //end of line
        {
            if (overflow)
            {
                Serial1.println("ERR LINE_TOO_LONG");
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

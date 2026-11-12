#pragma once

#include <Arduino.h>
#include "Config.h"

// 1: USB Serial Monitor; 0: external controller on Serial1 (RX/TX).
#define DEBUG_USE_USB_SERIAL 1

// These options apply only to USB debug; Serial1 keeps its timeout and protocol.
#define DEBUG_DISABLE_COMM_TIMEOUT 1
#define DEBUG_LOG_MOTOR_ACTIONS 1

namespace Debug
{
    constexpr bool communicationTimeoutEnabled =
        !(DEBUG_USE_USB_SERIAL && DEBUG_DISABLE_COMM_TIMEOUT);

    inline auto &port()
    {
#if DEBUG_USE_USB_SERIAL
        return Serial;
#else
        return Serial1;
#endif
    }

    inline void begin()
    {
#if DEBUG_USE_USB_SERIAL
        port().begin(Config::baud);
#else
        port().begin(Config::baud, SERIAL_8N1);
#endif
        // Do not wait for a USB connection: motor safety updates must run.
    }

    // Reports requested motor actions, not measured physical movement.
    inline void motorAction(const char *action)
    {
#if DEBUG_USE_USB_SERIAL && DEBUG_LOG_MOTOR_ACTIONS
        port().print("DEBUG ");
        port().println(action);
#else
        (void)action;
#endif
    }
}

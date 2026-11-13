#pragma once

#include <Arduino.h>
#include "Config.h"

// 1: USB Serial Monitor; 0: external controller on D0/D1.
// R3 shares Serial with USB; R4 uses Serial1 for D0/D1.
#ifndef DEBUG_USE_USB_SERIAL
#define DEBUG_USE_USB_SERIAL 1
#endif

// These options apply only to USB debug; controller mode keeps its timeout/protocol.
#ifndef DEBUG_DISABLE_COMM_TIMEOUT
#define DEBUG_DISABLE_COMM_TIMEOUT 1
#endif
#ifndef DEBUG_LOG_MOTOR_ACTIONS
#define DEBUG_LOG_MOTOR_ACTIONS 1
#endif

namespace Debug
{
    constexpr bool communicationTimeoutEnabled =
        !(DEBUG_USE_USB_SERIAL && DEBUG_DISABLE_COMM_TIMEOUT);

#if DEBUG_USE_USB_SERIAL || defined(ARDUINO_AVR_UNO)
    inline decltype(Serial) &port()
    {
        return Serial;
    }
#else
    inline decltype(Serial1) &port()
    {
        return Serial1;
    }
#endif

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

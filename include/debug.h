#pragma once

#include <Arduino.h>
#include "Config.h"

// Selected by the PlatformIO environment. Default to controller operation.
#ifndef APP_USB_DEBUG
#define APP_USB_DEBUG 0
#endif
#if APP_USB_DEBUG != 0 && APP_USB_DEBUG != 1
#error "APP_USB_DEBUG must be 0 (controller) or 1 (USB debug)"
#endif

namespace Debug
{
    constexpr bool communicationTimeoutEnabled =
        !APP_USB_DEBUG;
    constexpr const char *modeName = APP_USB_DEBUG ? "USB_DEBUG" : "CONTROLLER";

#if APP_USB_DEBUG || defined(ARDUINO_AVR_UNO)
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
#if APP_USB_DEBUG
        port().begin(Config::baud);
#else
        port().begin(Config::baud, SERIAL_8N1);
#endif
        // Do not wait for a USB connection: motor safety updates must run.
    }

    // Reports requested motor actions, not measured physical movement.
    inline void motorAction(const char *action)
    {
#if APP_USB_DEBUG
        port().print("DEBUG ");
        port().println(action);
#else
        (void)action;
#endif
    }
}

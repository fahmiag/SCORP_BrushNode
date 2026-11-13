#include <Arduino.h>
#include "Watchdog.h"

#if defined(ARDUINO_AVR_UNO)
#include <avr/wdt.h>

// After a watchdog reset, AVR can restart with the watchdog still running
// at its shortest interval. Clear WDRF and disable it before C++ startup.
// The bootloader must also support watchdog resets (standard Uno Optiboot does).
void disableWatchdogOnStartup() __attribute__((naked, section(".init3"), used));
void disableWatchdogOnStartup()
{
    MCUSR = 0;
    wdt_disable();
}

bool Watchdog::begin(unsigned long timeoutMs)
{
    if (timeoutMs == 0 || timeoutMs > 8000) return false;

    uint8_t interval;
    if (timeoutMs <= 15) interval = WDTO_15MS;
    else if (timeoutMs <= 30) interval = WDTO_30MS;
    else if (timeoutMs <= 60) interval = WDTO_60MS;
    else if (timeoutMs <= 120) interval = WDTO_120MS;
    else if (timeoutMs <= 250) interval = WDTO_250MS;
    else if (timeoutMs <= 500) interval = WDTO_500MS;
    else if (timeoutMs <= 1000) interval = WDTO_1S;
    else if (timeoutMs <= 2000) interval = WDTO_2S;
    else if (timeoutMs <= 4000) interval = WDTO_4S;
    else interval = WDTO_8S;

    wdt_reset();
    wdt_enable(interval);
    return true;
}

void Watchdog::refresh()
{
    wdt_reset();
}
#elif defined(ARDUINO_ARCH_RENESAS_UNO)
#include <WDT.h>

bool Watchdog::begin(unsigned long timeoutMs)
{
    return WDT.begin(timeoutMs);
}

void Watchdog::refresh()
{
    WDT.refresh();
}
#else
#error "Unsupported board: select Uno R3 or Uno R4."
#endif

#pragma once

namespace Watchdog
{
    bool begin(unsigned long timeoutMs);
    void refresh();
}

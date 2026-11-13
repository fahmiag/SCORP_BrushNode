# Brush motor node

Arduino UNO R3 or UNO R4 Minima, two Cytron MDD20A PWM/DIR channels, automatic-direction RS485 transceiver. Settings are in `include/Config.h`.

## Board selection and serial mode

Select `uno_r3` or `uno_r4_minima` in PlatformIO's project tasks for Build/Upload. The default is R4 Minima. From a PlatformIO terminal:

```sh
pio run -e uno_r3
pio run -e uno_r4_minima
pio run -e uno_r3 -t upload
pio run -e uno_r4_minima -t upload
```

The pin assignments below are the same on both boards. Each board needs its own compiled firmware. PWM frequency can differ between boards; verify motor behavior on each.

`include/debug.h` defaults to `DEBUG_USE_USB_SERIAL = 1` for the USB Serial Monitor, with communication timeout disabled and motor action logging enabled. Set it to `0` for the external RS485 controller: this enables the communication timeout and suppresses debug motor logs. These macros can also be overridden using PlatformIO `build_flags = -DDEBUG_USE_USB_SERIAL=0`.

R4 uses `Serial` for USB and `Serial1` for D0/D1. R3 uses `Serial` for both USB and D0/D1, so these cannot serve as independent channels. Disconnect/isolate the RS485 transceiver during R3 uploads and avoid simultaneous USB-host and RS485 transmissions. Controller mode still uses the D0/D1 wiring below.

## Placeholder wiring

| Signal | Pin |
| --- | --- |
| Motor PWM / DIR | D9 / D8 |
| Axis PWM / DIR | D10 / D11 |
| Top / bottom limit | A0 / A1 |
| Move Up / Down / Stop buttons | A2 / A3 / D7 |
| RS485 transceiver RO / DI | D0 (RX) / D1 (TX) |

Connect each normally open limit switch between its input and ground. Internal pullups are enabled. Connect controller, driver logic and transceiver grounds as required by their interfaces. Use an automatic transmit-direction RS485 module; the firmware does not control DE/RE. Confirm module voltage compatibility before connecting it.

## Watchdog and reset/startup inputs

The hardware watchdog starts after the motor control pins are driven low, before application initialization. `Config::watchdogTimeoutMs` requests 1000 ms. R4 uses the core WDT library; R3 uses the AVR hardware watchdog and selects the next supported nominal interval (maximum 8000 ms). Actual watchdog timing depends on the hardware oscillator. R3 clears the watchdog reset flag and disables the watchdog early in application startup before enabling the configured interval in `setup()`. Use a watchdog-compatible bootloader, such as standard Uno Optiboot; older clone bootloaders may loop after watchdog resets and must be verified or updated. Only a completed application loop refreshes the watchdog. A stalled loop causes a hardware reset; the communication timeout remains a separate fault mechanism. If watchdog initialization fails, firmware reports `FAULT WATCHDOG_INIT` and stays stopped without accepting commands. After a reset, motion is not restored: a new command is required.

Firmware cannot drive pins during reset, bootloader execution or loss of MCU power. Fit an external 10 kΩ pull-down from **each driver PWM and DIR input** to driver signal ground, preferably at the driver connector (four resistors total). PWM low is the required stop command; DIR low provides a defined direction while stopped. Verify these pull-downs keep the actual driver inputs below its specified low threshold with the MCU disconnected and with either supply powered first. Do not rely on software or MCU internal pulls for the reset interval. For the MDD20A, PWM low brakes the motor; it does not disconnect motor power.

At the start of `setup()`, both PWM pins are set low before either DIR pin is configured. Motor initialization also starts with zero PWM and a known low direction. The pin table above matches `Config.h`; update wiring and pull-down connections if those settings change.

Hardware verification (with motors unloaded): scope both PWM inputs during power-on, reset-button hold/release, firmware upload and MCU power loss while the driver remains powered. They must remain low until an explicit motion command. To test watchdog recovery, temporarily stall the main loop after starting motion, without refreshing WDT; verify a reset within the hardware interval and zero PWM throughout reboot, with no automatic restart. Remove the stall before normal operation. A software watchdog does not stop PWM immediately when code hangs; PWM may remain active until the watchdog reset. Use an independent driver inhibit or emergency-stop circuit if that interval is unacceptable.

## Protocol

Connect each normally open movement button between its pin and ground; internal pullups are enabled. A press starts upward (A2) or downward (A3) movement. Release does not stop motion. Press D7 to pause the axis; the brush is unaffected. Stop before selecting another direction. Pressing the same direction after a pause preserves elapsed travel time; selecting a different direction resets it. Upward movement retains the automatic top wait and return down. Limits and faults remain enforced. Buttons held at startup must be released before starting motion. Inputs act immediately on closure and debounce release for 20 ms. Stop or both direction buttons held together pauses movement and blocks starts; release and press a direction again to restart. Local buttons do not refresh the serial heartbeat or clear faults.

115200 baud, 8 data bits, no parity, 1 stop bit. Uppercase ASCII commands terminated by LF, CR or CRLF; no node address. Master must wait for the response before transmitting again on the half-duplex bus.

| Command | Action |
| --- | --- |
| `PING` | Heartbeat; replies `OK` |
| `MOTOR FORWARD` | Run first motor at full PWM in forward direction |
| `MOTOR REVERSE` | Run first motor at full PWM in reverse direction |
| `MOTOR OFF` | Stop first motor |
| `AXIS START` | Start the cycle or resume the paused movement / top wait |
| `AXIS STOP` | Immediately pause the axis; remember movement and elapsed time |
| `AXIS UP` | Select upward direction while idle or paused; send `AXIS START` to move |
| `AXIS DOWN` | Select downward direction while idle or paused; send `AXIS START` to move |
| `STOP` | Stop both motors and cancel cycle |
| `STATUS` | Report axis state, active limits and fault flag |
| `CLEAR` | Stop both motors and clear fault if both limits are not active |

Send `PING` every second, including during the top wait. Any recognized command refreshes the communication timer. After 3 seconds without a recognized command, both motors stop and a fault latches. Reconnection never resumes motion automatically: send `CLEAR`, then a new motion command.

Each axis travel leg has its own 120-second timeout. Travel timeout or simultaneous active limits stops both motors and latches a fault. Stops use zero PWM. On power-up both motors remain stopped. Starting a cycle while already at the top begins the 2-second wait; starting elsewhere moves upward first. Repeated `AXIS START` while busy returns an error and does not restart the timer. There is no automatic repetition.

To reverse during a movement, send `AXIS STOP`, then `AXIS DOWN` or `AXIS UP`, then `AXIS START`. Direction commands do not start motion and return `ERR AXIS_STOP_REQUIRED` during an active cycle (including the automatic top wait). Selecting a direction resets the travel timer; resuming without a direction command preserves elapsed travel or wait time and excludes time spent paused. Upward movement still waits at the top and then returns down automatically. Downward movement ends at the bottom; starting downward while the bottom limit is active returns `ERR AXIS_NOT_READY`. `STOP`, `CLEAR` and faults cancel the saved movement and reset the next cycle to upward. `STATUS` includes `PAUSED` and `DIR=UP` or `DIR=DOWN`. Keep sending heartbeats while paused.

Successful commands reply `OK`, status replies `STATUS ...`, invalid/rejected commands reply `ERR ...`. Faults emit `FAULT ...`, and arrival at the bottom emits `EVENT CYCLE_DONE`; the master must accommodate these asynchronous messages.

Verify direction polarity with unloaded motors before operation; change `forwardLevel` and `upLevel` if required. Both motors ramp to configurable target PWM (default 255). Physical travel, wiring, motor reversal and transceiver timing require hardware verification. Normally open switches cannot reveal a broken wire; the travel timeout bounds a missing-switch run.

## Source layout and ramp settings

- `main.cpp`: Arduino entry points.
- `App.cpp`: serial commands, heartbeat and application scheduling.
- `Statemachine.cpp`: axis sequence, limit switches and fault handling.
- `AxisMotor.cpp` / `brushMotor.cpp`: separate motor instances and interfaces.
- `RampedMotor.cpp`: shared nonblocking PWM ramp and reversal handling.
- `include/Config.h`: pins, direction polarity, speed and timing settings.

Both motors ramp linearly from zero to full PWM over `rampMs` (default 1000 ms). `axisSpeed` and `brushSpeed` set targets from 0 to 255. A direction change ramps PWM down to zero, waits `reversalPauseMs` (default 100 ms), changes DIR, then ramps up. Repeating a running command does not restart the ramp. Set `rampMs` to zero to disable the ramp. PWM is an output command, not measured motor speed.

`STOP`, `MOTOR OFF`, `AXIS STOP`, faults and limit activation stop the relevant PWM immediately and cancel pending ramps. Travel timeout includes ramp time. All timing is nonblocking so serial commands and limits continue to be checked during ramps.

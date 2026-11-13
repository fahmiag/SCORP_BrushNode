# Brush motor node

Arduino UNO R3 or UNO R4 Minima, two Cytron MDD20A PWM/DIR channels, automatic-direction RS485 transceiver. Settings are in `include/Config.h`.

## Board selection and serial mode

In VS Code, open PlatformIO > PROJECT TASKS, expand the environment below, then choose General > Build or Upload. The default is R4 Minima in controller mode.

| Environment | Board | Mode | Communication timeout | Motor action logs |
| --- | --- | --- | --- | --- |
| `uno_r3` | Uno R3 | Controller (D0/D1) | Enabled | Off |
| `uno_r4_minima` | Uno R4 Minima | Controller (D0/D1) | Enabled | Off |
| `uno_r3_debug` | Uno R3 | USB debug | Disabled | On |
| `uno_r4_minima_debug` | Uno R4 Minima | USB debug | Disabled | On |

Set `default_envs` in `platformio.ini` to the desired environment for the toolbar Build/Upload buttons. From a PlatformIO terminal:

```sh
pio run -e uno_r3
pio run -e uno_r4_minima
pio run -e uno_r3 -t upload
pio run -e uno_r4_minima -t upload
```

The pin assignments below are the same on both boards. Each board needs its own compiled firmware. PWM frequency can differ between boards; verify motor behavior on each.

Operating mode is chosen at build time using `APP_USB_DEBUG` in `platformio.ini`: `0` selects controller mode, `1` selects USB debug. Select an environment instead of editing `include/debug.h`. The former `DEBUG_USE_USB_SERIAL`, `DEBUG_DISABLE_COMM_TIMEOUT` and `DEBUG_LOG_MOTOR_ACTIONS` settings are replaced by this single mode setting. Watchdog, limits and travel timeout remain enabled in both modes. In controller mode the communication timeout starts after the first recognized command; local buttons do not establish a serial connection or refresh its timer.

R4 uses `Serial` for USB and `Serial1` for D0/D1. R3 uses `Serial` for both USB and D0/D1, so these cannot serve as independent channels. Disconnect/isolate the RS485 transceiver during R3 uploads and avoid simultaneous USB-host and RS485 transmissions. Controller mode still uses the D0/D1 wiring below.

## Placeholder wiring

| Signal | Pin |
| --- | --- |
| Motor PWM / DIR | D9 / D8 |
| Axis PWM / DIR | D10 / D11 |
| Top / bottom limit | A0 / A1 |
| Move Up / Down / Stop buttons | A2 / A3 / D7 |
| RS485 transceiver RO / DI | D0 (RX) / D1 (TX) |

Connect each limit switch using its **COM and NC (normally closed)** terminals: COM to ground, NC to A0 (top) or A1 (bottom). Leave NO disconnected. Internal pullups are enabled. An unactuated switch closes the circuit to ground (LOW, inactive); actuating it opens the circuit (HIGH, active). A disconnected wire also reads as an active limit. Limit activation is immediate; returning to inactive is debounced for 20 ms. Both limits are sampled during startup.

The firmware cannot distinguish an actuated limit from a broken wire. A single open circuit follows the normal limit behavior, including the top wait and return downward; both open circuits cause `BOTH_LIMITS`. A short to ground can mask limit activation. With motors unloaded, verify `STATUS` reports `TOP=0 BOTTOM=0` with both switches unactuated, the corresponding field becomes `1` when each switch is actuated or disconnected, and opening both latches `BOTH_LIMITS`. Movement buttons remain normally open.

Connect controller, driver logic and transceiver grounds as required by their interfaces. Use an automatic transmit-direction RS485 module; the firmware does not control DE/RE. Confirm module voltage compatibility before connecting it.

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
| `BRUSH FORWARD` | Run brush motor at full PWM in forward direction |
| `BRUSH REVERSE` | Run brush motor at full PWM in reverse direction |
| `BRUSH OFF` | Stop brush motor |
| `MOVE START` | Start the cycle or resume the paused movement / top wait |
| `MOVE STOP` | Immediately pause the axis; remember movement and elapsed time |
| `MOVE UP` | Select upward direction while idle or paused; send `MOVE START` to move |
| `MOVE DOWN` | Select downward direction while idle or paused; send `MOVE START` to move |
| `STOP` | Stop both motors and cancel cycle |
| `STATUS` | Report axis state, active limits, fault flag, latched fault reason and operating mode |
| `CLEAR` | Stop both motors and clear fault if both limits are not active |

Send `PING` every second, including during the top wait. Any recognized command refreshes the communication timer. After 3 seconds without a recognized command, both motors stop and a fault latches. Reconnection never resumes motion automatically: send `CLEAR`, then a new motion command.

Each axis travel leg has its own 120-second timeout. Travel timeout or simultaneous active limits stops both motors and latches a fault. Stops use zero PWM. On power-up both motors remain stopped. Starting a cycle while already at the top begins the 2-second wait; starting elsewhere moves upward first. Repeated `MOVE START` while busy returns an error and does not restart the timer. There is no automatic repetition.

To reverse during a movement, send `MOVE STOP`, then `MOVE DOWN` or `MOVE UP`, then `MOVE START`. Direction commands do not start motion and return `ERR MOVE_STOP_REQUIRED` during an active cycle (including the automatic top wait). Selecting a direction resets the travel timer; resuming without a direction command preserves elapsed travel or wait time and excludes time spent paused. Upward movement still waits at the top and then returns down automatically. Downward movement ends at the bottom; starting downward while the bottom limit is active returns `ERR MOVE_NOT_READY`. `STOP`, `CLEAR` and faults cancel the saved movement and reset the next cycle to upward. `STATUS` includes `PAUSED` and `DIR=UP` or `DIR=DOWN`. Keep sending heartbeats while paused.

Successful commands reply `OK`, status replies `STATUS ...`, invalid/rejected commands reply `ERR ...`. Faults emit `FAULT ...`, and arrival at the bottom emits `EVENT CYCLE_DONE`; the master must accommodate these asynchronous messages.

`STATUS` keeps its existing fields and appends `REASON` and `MODE`, for example:

```text
STATUS MOVE=IDLE DIR=UP TOP=0 BOTTOM=0 FAULT=1 REASON=UP_TIMEOUT MODE=CONTROLLER
```

`REASON` is `NONE`, `BOTH_LIMITS`, `UP_TIMEOUT`, `DOWN_TIMEOUT` or `COMM_TIMEOUT`. The first fault reason remains latched until a successful `CLEAR`; subsequent trips still stop both motors but do not replace the reason or repeat the fault notification. `STOP`, reconnecting and querying status do not clear it. `CLEAR` refuses while both limits are active. Fault history is held in RAM and resets on reboot. `MODE` is `CONTROLLER` or `USB_DEBUG`. Watchdog initialization failure remains a separate startup `FAULT WATCHDOG_INIT` message, because the application cannot accept commands in that state.

To verify on hardware with unloaded motors: query `STATUS` in each build and check `MODE`; in controller mode send `PING`, wait beyond `communicationTimeoutMs`, then check `FAULT=1 REASON=COMM_TIMEOUT`. In USB debug, the same wait must not cause a communication fault. Trigger both limits and check `BOTH_LIMITS`; allow a later communication timeout and confirm the original reason remains. `STOP` must preserve that reason, `CLEAR` must refuse with both limits active, and releasing the limits then sending `CLEAR` must restore `FAULT=0 REASON=NONE`. Verify each travel timeout reports its corresponding reason.

Verify direction polarity with unloaded motors before operation; change `forwardLevel` and `upLevel` if required. Both motors ramp to configurable target PWM (default 255). Physical travel, wiring, motor reversal and transceiver timing require hardware verification. Normally closed limits treat an open wire as active; the travel timeout also bounds a run if a switch fails to open.

## Source layout and ramp settings

- `main.cpp`: Arduino entry points.
- `App.cpp`: serial commands, heartbeat and application scheduling.
- `Statemachine.cpp`: axis sequence, limit switches and fault handling.
- `AxisMotor.cpp` / `brushMotor.cpp`: separate motor instances and interfaces.
- `RampedMotor.cpp`: shared nonblocking PWM ramp and reversal handling.
- `include/Config.h`: pins, direction polarity, speed and timing settings.

Both motors ramp linearly from zero to full PWM over `rampMs` (default 1000 ms). `axisSpeed` and `brushSpeed` set targets from 0 to 255. A direction change ramps PWM down to zero, waits `reversalPauseMs` (default 100 ms), changes DIR, then ramps up. Repeating a running command does not restart the ramp. Set `rampMs` to zero to disable the ramp. PWM is an output command, not measured motor speed.

`STOP`, `BRUSH OFF`, `MOVE STOP`, faults and limit activation stop the relevant PWM immediately and cancel pending ramps. Travel timeout includes ramp time. All timing is nonblocking so serial commands and limits continue to be checked during ramps.

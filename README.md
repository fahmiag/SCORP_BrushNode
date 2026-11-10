# Brush motor node

Arduino UNO R4 Minima, two Cytron MDD20A PWM/DIR channels, automatic-direction RS485 transceiver. Settings are in `include/Config.h`.

## Placeholder wiring

| Signal | Pin |
| --- | --- |
| Motor PWM / DIR | D5 / D4 |
| Axis PWM / DIR | D6 / D7 |
| Top / bottom limit | D8 / D9 |
| RS485 transceiver RO / DI | D0 (RX) / D1 (TX) |

Connect each normally open limit switch between its input and ground. Internal pullups are enabled. Connect controller, driver logic and transceiver grounds as required by their interfaces. Use an automatic transmit-direction RS485 module; the firmware does not control DE/RE. Confirm module voltage compatibility before connecting it.

## Protocol

115200 baud, 8 data bits, no parity, 1 stop bit. Uppercase ASCII commands terminated by LF, CR or CRLF; no node address. Master must wait for the response before transmitting again on the half-duplex bus.

| Command | Action |
| --- | --- |
| `PING` | Heartbeat; replies `OK` |
| `MOTOR FORWARD` | Run first motor at full PWM in forward direction |
| `MOTOR REVERSE` | Run first motor at full PWM in reverse direction |
| `MOTOR OFF` | Stop first motor |
| `AXIS START` | Move up, wait 2 seconds at top, move down, stop at bottom |
| `AXIS STOP` | Cancel axis cycle |
| `STOP` | Stop both motors and cancel cycle |
| `STATUS` | Report axis state, active limits and fault flag |
| `CLEAR` | Stop both motors and clear fault if both limits are not active |

Send `PING` every second, including during the top wait. Any recognized command refreshes the communication timer. After 3 seconds without a recognized command, both motors stop and a fault latches. Reconnection never resumes motion automatically: send `CLEAR`, then a new motion command.

Each axis travel leg has its own 120-second timeout. Travel timeout or simultaneous active limits stops both motors and latches a fault. Stops use zero PWM. On power-up both motors remain stopped. Starting a cycle while already at the top begins the 2-second wait; starting elsewhere moves upward first. Repeated `AXIS START` while busy returns an error and does not restart the timer. There is no automatic repetition.

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

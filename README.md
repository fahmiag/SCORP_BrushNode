# Brush motor node

Arduino UNO R4 Minima, two Cytron MDD20A PWM/DIR channels, automatic-direction RS485 transceiver. Settings are in `Config` in `src/main.cpp`.

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

Verify direction polarity with unloaded motors before operation; change `forwardLevel` and `upLevel` if required. Both motors currently use full PWM (255). Physical travel, wiring, motor reversal and transceiver timing require hardware verification. Normally open switches cannot reveal a broken wire; the travel timeout bounds a missing-switch run.

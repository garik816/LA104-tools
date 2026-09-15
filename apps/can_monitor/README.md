# CAN Monitor for LA104

Passive CAN monitor for LA104's STM32 bxCAN controller.

## Wiring and safety

Use an external **3.3 V CAN transceiver** such as SN65HVD230 or a compatible isolated module:

- LA104 `P3` -> transceiver `RXD` (CAN receive)
- LA104 `P4` -> transceiver `TXD` (CAN transmit)
- common logic GND
- transceiver `CANH`/`CANL` -> bus

Never connect LA104 GPIO directly to CANH/CANL. The app listens in bxCAN silent mode: it does not transmit and does not ACK frames.

## Features

- Auto-baud scan: 125k, 250k, 500k and 1M bit/s.
- Passive raw-frame monitor for standard and extended IDs.
- Observed-ID list and frame/error counters.
- Conservative protocol hints: CANopen and J1939 candidates only when corresponding traffic patterns are seen.

Auto-baud requires an already active bus. It cannot discover a silent device or prove a protocol from one frame.

## Controls

- `F1` — manually select next baud rate
- `F3` — pause/resume capture
- `F4` — restart auto-baud
- `F2` — exit


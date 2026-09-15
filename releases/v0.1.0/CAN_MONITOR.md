# CAN Monitor v0.1.0

- File: `142canmon.elf`
- Target: LA104 with gabonator/LA104 firmware
- Size: 6945 bytes
- SHA-256: `3445228d7b19732e695418583099f2bad6832e1073763bebdb610857a3ff3794`
- CAN controller: STM32 bxCAN, passive/silent receive mode
- Wiring: P3=CAN_RX, P4=CAN_TX through a 3.3 V external CAN transceiver

Copy the ELF file to LA104 storage and launch it from the firmware file manager. The monitor never transmits or ACKs frames.

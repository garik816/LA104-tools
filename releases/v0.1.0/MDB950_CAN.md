# Hitec MDB950SW-CAN Doctor v0.1.0

- File: 143mdbcan.elf
- Target: LA104 + gabonator/LA104 firmware
- Size: 7169 bytes
- SHA-256: e4ed8c38452c06b71acbc39e20eddd10c876b37f497218d29fc5e97cee6fb348
- CAN: Hitec CAN 2.0A, default target ID2=0; probe sequence starts at 250 kbit/s.

The app reads diagnostics using Hitec r/R requests. It does not save, reset, factory-reset or change configuration. Motion is one volatile REG_POSITION_NEW command only after F2 ARM and F3/F4; it requires a discovered, non-zero ID1 and current position.

Use a 3.3 V external CAN transceiver. P3=CAN_RX and P4=CAN_TX. Never connect CANH/CANL directly to LA104 GPIO.
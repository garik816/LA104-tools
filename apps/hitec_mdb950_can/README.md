# Hitec MDB950SW-CAN Doctor

LA104 application for a Hitec MDB950SW-CAN configured for CAN 2.0A and an external 3.3 V CAN transceiver.

## Wiring

- LA104 P3 (PB8) -> transceiver RXD / CAN RX
- LA104 P4 (PB9) -> transceiver TXD / CAN TX
- common GND, and transceiver CANH/CANL -> the bus

Do **not** connect CANH or CANL directly to an LA104 probe input. Power the servo separately and correctly terminate the CAN bus.

## What it does

- actively scans non-zero Hitec ID1 values at CAN ID2=0, first at 250 kbit/s and then 500/125/1000 kbit/s;
- uses only read packets (`r`/`R`) for identification and live diagnostics;
- reads position, supply voltage, MCU temperature, motor current and firmware version;
- issues no save, reset, factory-default, configuration or broadcast write command;
- offers a single **explicitly armed** relative position test: F2 then F3/F4 sends one volatile `REG_POSITION_NEW` command for about -2/+2 degrees.

The scanner deliberately excludes ID1=0. Hitec defines it as broadcast, and multiple unconfigured servos could answer at once. Assign a unique non-zero ID1 with Hitec DPC-CAN before using this utility on a shared bus.

The application assumes CAN 2.0A and ID2=0 (the documented default/unset target CAN ID). For a servo with a configured non-zero ID2, use the passive CAN Monitor first and extend/configure the target CAN ID only after observing the bus.

## Build

Run `build.sh` in a bash environment with `arm-none-eabi-gcc` in `PATH`. The deployable file is `build/143mdbcan.elf`.

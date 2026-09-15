# LA104 Tools

Practical LA104 applications for gabonator/LA104 firmware.

## Applications

- [I2C Sensor Tester](apps/i2c_sensor_tester) — I2C scanner and diagnostics for BMP280/BME280, DS3231, INA219, SSD1306/SH1106.
- [CAN Monitor](apps/can_monitor) — passive CAN monitor: auto-baud, raw frames and CANopen/J1939 hints.
- [Hitec MDB950SW-CAN Doctor](apps/hitec_mdb950_can) — diagnostics and explicitly armed limited test for Hitec MDB950SW-CAN.
- [Servo Protocol Probe](apps/servo_protocol_probe) — passive automatic recognition of Hitec Custom, DroneCAN/UAVCAN v0, CANopen and J1939 traffic.

## Release binaries

- [I2C Sensor Tester v0.1.0](releases/v0.1.0/141i2ct.elf) — 4,584 bytes; SHA-256: ac1a0094bd38662e77d7e2603255d47c9e8ddf82bc289958522494bc7164311c.
- [CAN Monitor v0.1.0](releases/v0.1.0/142canmon.elf) — 6,945 bytes; SHA-256: 3445228d7b19732e695418583099f2bad6832e1073763bebdb610857a3ff3794.
- [Hitec MDB950SW-CAN Doctor v0.1.0](releases/v0.1.0/143mdbcan.elf) — 7,169 bytes; SHA-256: e4ed8c38452c06b71acbc39e20eddd10c876b37f497218d29fc5e97cee6fb348.
- [Servo Protocol Probe v0.1.0](releases/v0.1.0/144servoprobe.elf) — 6,883 bytes; SHA-256: 0acb84c1dda98c79fd5b65fb8303d0046a144b1c72803c3bd9cc53f32da9eb62.

All CAN applications require an external 3.3 V CAN transceiver: LA104 P3=CAN_RX, P4=CAN_TX. Never connect CANH/CANL directly to LA104 probes.
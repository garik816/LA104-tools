# LA104 Tools

Практические диагностические приложения для логического анализатора **LA104** с прошивкой [gabonator/LA104](https://github.com/gabonator/LA104).

Каждая утилита — самостоятельное приложение. Код в этом репозитории является overlay для upstream-проекта: скопируйте каталог нужного приложения в `system/apps_featured/` локальной копии LA104 и соберите его штатным ARM toolchain.

## Applications

- [`apps/i2c_sensor_tester`](apps/i2c_sensor_tester) — I2C Scanner и безопасная диагностика BMP280/BME280, DS3231, INA219, SSD1306/SH1106.
- [`apps/can_monitor`](apps/can_monitor) — passive CAN monitor: auto-baud, raw frames, CANopen/J1939 candidates.

## Release binaries

- [I2C Sensor Tester v0.1.0](releases/v0.1.0/141i2ct.elf) — 4,584 bytes; SHA-256: `ac1a0094bd38662e77d7e2603255d47c9e8ddf82bc289958522494bc7164311c`.
- [CAN Monitor v0.1.0](releases/v0.1.0/142canmon.elf) — 6,945 bytes; SHA-256: `3445228d7b19732e695418583099f2bad6832e1073763bebdb610857a3ff3794`.

## Current status

Release binaries are compiled with Arm GNU Toolchain 14.2.Rel1. CAN Monitor needs a 3.3 V external CAN transceiver; never connect LA104 GPIO directly to CANH/CANL.

# I2C Sensor Tester for LA104

Standalone, read-only diagnostic tool for the LA104 GPIO I2C bus:

- Scan standard 7-bit I2C addresses (`0x08`–`0x77`).
- BMP280/BME280: definitive chip-ID probe, raw temperature/pressure/humidity and compensated temperature.
- DS3231: date/time, temperature and status register (reported as a compatible candidate because the IC has no ID register).
- INA219: configuration, bus voltage and shunt voltage (reported as a compatible candidate because the IC has no ID register).
- SSD1306/SH1106: address-ACK detection at `0x3c`/`0x3d`; no display commands are sent.
- Other ACKing devices remain explicitly labelled `Unknown`.

## Wiring

`P1` is SCL and `P2` is SDA. Connect a common ground and use suitable pull-ups to the peripheral's logic voltage. Do not connect a 5 V I2C bus directly to LA104 GPIO.

## Controls

- `F1`: scan again
- `Up` / `Down`: select a found address
- `F3`: re-read the selected device
- `F2`: exit

## Build

From this directory in a POSIX shell:

```sh
./build.sh
```

Build the repository's BIOS import library first, from `system/os_library`:

```sh
./build.sh
```

Then copy `build/141i2ct.elf` to the LA104 storage. The app build follows the repository's existing bare-metal pattern and needs `arm-none-eabi-g++` plus `system/os_library/build/libbios_la104.so`.


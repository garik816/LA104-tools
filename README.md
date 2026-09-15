# LA104 Tools

Практические диагностические приложения для логического анализатора **LA104** с прошивкой [gabonator/LA104](https://github.com/gabonator/LA104).

Каждая утилита — самостоятельное приложение. Код в этом репозитории является overlay для upstream-проекта: скопируйте каталог нужного приложения в `system/apps_featured/` локальной копии LA104 и соберите его штатным ARM toolchain.

## Applications

- [`apps/i2c_sensor_tester`](apps/i2c_sensor_tester) — I2C Scanner и безопасная диагностика BMP280/BME280, DS3231, INA219, SSD1306/SH1106.

## Current status

Первое приложение добавлено исходным кодом. Для создания ELF нужен `arm-none-eabi-g++` и предварительно собранная библиотека BIOS из `system/os_library` upstream-репозитория.

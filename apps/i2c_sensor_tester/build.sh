#!/usr/bin/env bash
# Requires arm-none-eabi-g++ and a previously built system/os_library/build/libbios_la104.so
set -e

mkdir -p build
cd build

arm-none-eabi-g++ -Wall -Os -Werror -fno-common -mcpu=cortex-m3 -mthumb -msoft-float \
  -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit -Wno-psabi \
  -DLA104 -D_ARM -DSTM32F10X_HD -MD -c ../main.cpp \
  ../../../os_host/source/framework/Serialize.cpp \
  -I../../../os_library/include/ \
  -I../../../os_host/lib/CMSIS/Device/STM32F10x/Include \
  -I../../../os_host/lib/STM32F10x_StdPeriph_Driver/inc \
  -I../../../os_host/lib/CMSIS/Include

arm-none-eabi-gcc -fPIC -mcpu=cortex-m3 -mthumb -o output.elf -nostartfiles \
  -T ../app.lds ./main.o ./Serialize.o -lbios_la104 -L../../../os_library/build

arm-none-eabi-objdump -d -S output.elf > output.asm
arm-none-eabi-readelf -all output.elf > output.txt

find . -type f -name '*.o' -delete
find . -type f -name '*.d' -delete

../../../../tools/elfstrip/elfstrip output.elf 141i2ct.elf


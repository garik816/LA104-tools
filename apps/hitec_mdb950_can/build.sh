#!/usr/bin/env bash
set -e

mkdir -p build
cd build

ROOT=../../../
STDLIB=$ROOT/os_host/library

arm-none-eabi-g++ -std=gnu++11 -Wall -Os -Werror -fno-common -mcpu=cortex-m3 -mthumb -msoft-float \
  -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit -Wno-psabi \
  -DLA104 -D_ARM -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER -MD -c \
  ../main.cpp ../can.cpp $ROOT/os_host/source/framework/Serialize.cpp \
  -I$ROOT/os_library/include -I$STDLIB/STM32F10x_StdPeriph_Driver/inc \
  -I$ROOT/os_host/source -I$STDLIB/CMSIS/Include -I$STDLIB/CMSIS/Device/STM32F10x/Include

arm-none-eabi-gcc -Wall -Os -Werror -fno-common -mcpu=cortex-m3 -mthumb -msoft-float \
  -DLA104 -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER -c \
  $STDLIB/STM32F10x_StdPeriph_Driver/src/stm32f10x_can.c \
  $STDLIB/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
  $STDLIB/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
  -I$ROOT/os_host/source -I$STDLIB/STM32F10x_StdPeriph_Driver/inc -I$STDLIB/CMSIS/Include \
  -I$STDLIB/CMSIS/Device/STM32F10x/Include

arm-none-eabi-gcc -fPIC -mcpu=cortex-m3 -mthumb -o output.elf -nostartfiles -T ../app.lds \
  main.o can.o Serialize.o stm32f10x_can.o stm32f10x_gpio.o stm32f10x_rcc.o \
  -lbios_la104 -L$ROOT/os_library/build

arm-none-eabi-objdump -d -S output.elf > output.asm
arm-none-eabi-readelf -all output.elf > output.txt
../../../../tools/elfstrip/elfstrip output.elf 143mdbcan.elf

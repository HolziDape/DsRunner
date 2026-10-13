#!/bin/sh
# Builds testrom/dsrunner-test.gba with libgba inside the pinned devkitARM image.
set -e
DEVKITARM_IMAGE="${DEVKITARM_IMAGE:-devkitpro/devkitarm:20200730}"
if [ -z "$DEVKITARM" ]; then
	exec docker run --rm -v "$(pwd)":/testrom -w /testrom "$DEVKITARM_IMAGE" sh ./build.sh
fi
export PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"
arm-none-eabi-gcc -O2 -mthumb -mthumb-interwork -mcpu=arm7tdmi -specs=gba.specs \
	-I"$DEVKITPRO/libgba/include" main.c -L"$DEVKITPRO/libgba/lib" -lgba -o dsrunner-test.elf
arm-none-eabi-objcopy -O binary dsrunner-test.elf dsrunner-test.gba
gbafix dsrunner-test.gba -tDSRUNNERTEST -cDSRT -mHD
rm dsrunner-test.elf
ls -l dsrunner-test.gba

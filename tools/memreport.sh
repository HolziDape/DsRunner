#!/bin/sh
# Prints how much of the limited ARM9 memory regions the current build uses.
# Must run inside the devkitARM container after a successful make.
OBJDUMP="$DEVKITARM/bin/arm-none-eabi-objdump"
ELF="${1:-arm9/$(basename "$(pwd)").elf}"

vram=0; itcm=0; dtcm=0; ewram=0
# objdump -h section lines: idx name size vma lma offset align
"$OBJDUMP" -h "$ELF" | grep -E '^ +[0-9]+ ' > /tmp/sections.txt
while read -r idx name size vma rest; do
	size=$((0x$size)); vma=$((0x$vma))
	if   [ $vma -ge $((0x06800000)) ] && [ $vma -lt $((0x06840000)) ]; then vram=$((vram + size))
	elif [ $vma -ge $((0x01000000)) ] && [ $vma -lt $((0x01008000)) ]; then itcm=$((itcm + size))
	elif [ $vma -ge $((0x0b000000)) ] && [ $vma -lt $((0x0b004000)) ]; then dtcm=$((dtcm + size))
	elif [ $vma -ge $((0x02000000)) ] && [ $vma -lt $((0x02400000)) ]; then ewram=$((ewram + size))
	fi
done < /tmp/sections.txt

printf "  VRAM  %7d / 262144 bytes (%d%%)\n" $vram $((vram * 100 / 262144))
printf "  ITCM  %7d /  32768 bytes (%d%%)\n" $itcm $((itcm * 100 / 32768))
printf "  DTCM  %7d /  16384 bytes (%d%%)\n" $dtcm $((dtcm * 100 / 16384))
printf "  EWRAM %7d bytes\n" $ewram

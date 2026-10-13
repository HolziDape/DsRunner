// DsRunner test rom: a tiny GBA program to check a DsRunner build without
// commercial games. It checks booting, text output, vblank irq, timers, keys and
// SRAM saving (boot counter that has to survive a restart).
#include <gba.h>
#include <stdio.h>

// save type tag, found by DsRunner's save detection (no code patch needed)
__attribute__((aligned(4), used)) const char gSaveTag[] = "SRAM_F_V100";

#define SRAM_BYTES ((volatile u8*)SRAM)

static const u8 sMagic[4] = { 'D', 'S', 'R', 'T' };

static u32 sramRead32(int offset)
{
	return SRAM_BYTES[offset] | (SRAM_BYTES[offset + 1] << 8) | (SRAM_BYTES[offset + 2] << 16) | (SRAM_BYTES[offset + 3] << 24);
}

static void sramWrite32(int offset, u32 value)
{
	for (int i = 0; i < 4; i++)
		SRAM_BYTES[offset + i] = (value >> (i * 8)) & 0xFF;
}

static u32 updateBootCounter(void)
{
	bool valid = true;
	for (int i = 0; i < 4; i++)
		if (SRAM_BYTES[i] != sMagic[i])
			valid = false;
	u32 count = valid ? sramRead32(4) : 0;
	count++;
	for (int i = 0; i < 4; i++)
		SRAM_BYTES[i] = sMagic[i];
	sramWrite32(4, count);
	return count;
}

int main(void)
{
	irqInit();
	irqEnable(IRQ_VBLANK);
	consoleDemoInit();

	// timer 0 overflows once per second (16384 Hz, reload 65536 - 16384),
	// timer 1 counts the overflows
	REG_TM0CNT_L = 65536 - 16384;
	REG_TM0CNT_H = TIMER_START | 3;
	REG_TM1CNT_L = 0;
	REG_TM1CNT_H = TIMER_START | TIMER_COUNT;

	u32 boots = updateBootCounter();

	iprintf("\x1b[1;1HDsRunner test rom\n");
	iprintf("\x1b[3;1HBoot count (SRAM): %lu\n", boots);
	iprintf("\x1b[4;1H%s\n", boots > 1 ? "Save OK (restored)" : "First boot / no save yet");

	u32 frames = 0;
	while (1)
	{
		VBlankIntrWait();
		frames++;
		scanKeys();
		u16 held = keysHeld();
		u32 ticks = REG_TM1CNT_L;
		iprintf("\x1b[6;1HFrames (vblank irq): %lu   ", frames);
		iprintf("\x1b[7;1HTimer seconds:       %lu   ", ticks);
		iprintf("\x1b[9;1HKeys: %c%c%c%c %c%c%c%c %c%c   ",
			held & KEY_UP ? 'U' : '-', held & KEY_DOWN ? 'D' : '-',
			held & KEY_LEFT ? 'L' : '-', held & KEY_RIGHT ? 'R' : '-',
			held & KEY_A ? 'A' : '-', held & KEY_B ? 'B' : '-',
			held & KEY_L ? 'l' : '-', held & KEY_R ? 'r' : '-',
			held & KEY_START ? 'S' : '-', held & KEY_SELECT ? 's' : '-');
		// frames / seconds should be close to 59.7 if timing is right
		if (ticks > 0)
			iprintf("\x1b[11;1HFPS: %lu   ", frames / ticks);
	}
}

#ifndef __FIFO_H__
#define __FIFO_H__

#define FIFO_CNT_EMPTY	(1 << 8)

#define REG_FIFO_CNT	(*((vu32*)0x04000184))
#define REG_SEND_FIFO	(*((vu32*)0x04000188))
#define REG_RECV_FIFO	(*((vu32*)0x04100000))

//DS Lite backlight: arm9 sends the command followed by a level (0-3) or
//FIFO_BACKLIGHT_QUERY, arm7 replies with the current level or FIFO_BACKLIGHT_UNSUPPORTED
#define FIFO_CMD_BACKLIGHT			0xAA550101
#define FIFO_BACKLIGHT_QUERY		0xFF
#define FIFO_BACKLIGHT_UNSUPPORTED	0xFFFFFFFF

#endif
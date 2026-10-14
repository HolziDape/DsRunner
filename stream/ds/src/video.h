// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: screen layout.
//
// Both engines run mode 5: BG0 is a text console, BG3 an 8 bpp 256x256 bitmap
// (at 64 KB in VRAM A for the main engine, VRAM C for the sub engine).
// Main engine = info console / picture, sub engine = status console / lower half
// of the picture in the two-screen mode.

#ifndef DSSTREAM_VIDEO_H
#define DSSTREAM_VIDEO_H

#include <nds.h>

#include "decode.h"

extern PrintConsole con_info;    // main engine
extern PrintConsole con_status;  // sub engine

void video_init(void);
// Menu layout: info on top, status on the bottom (touch) screen, no picture.
void video_menu(void);
// Picture layout for a mode. on_top: one-screen picture on the top screen.
void video_stream(uint8_t mode, bool on_top);
const DecodeTarget *video_target(void);
// Black picture (before a new stream).
void video_clear(void);

#endif

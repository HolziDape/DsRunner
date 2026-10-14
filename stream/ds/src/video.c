// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: screen layout.

#include <nds.h>

#include "video.h"

PrintConsole con_info;
PrintConsole con_status;

static int bg_main, bg_sub;
static DecodeTarget target;

// consoleInit() also loads the font palette, which the picture overwrites
static void init_info_console(void)
{
    // map at 0 KB, font at 16 KB, the bitmap starts at 64 KB
    consoleInit(&con_info, 0, BgType_Text4bpp, BgSize_T_256x256, 0, 1, true, true);
    bgSetPriority(con_info.bgId, 0);
}

static void init_status_console(void)
{
    consoleInit(&con_status, 0, BgType_Text4bpp, BgSize_T_256x256, 0, 1, false, true);
    bgSetPriority(con_status.bgId, 0);
}

static void set_scale(bool sub, int s)
{
    // s in 8.8 fixed point: 256 = 1:1, 128 = magnify 2x
    if (sub)
    {
        REG_BG3PA_SUB = s; REG_BG3PB_SUB = 0; REG_BG3PC_SUB = 0; REG_BG3PD_SUB = s;
        REG_BG3X_SUB = 0; REG_BG3Y_SUB = 0;
    }
    else
    {
        REG_BG3PA = s; REG_BG3PB = 0; REG_BG3PC = 0; REG_BG3PD = s;
        REG_BG3X = 0; REG_BG3Y = 0;
    }
}

void video_init(void)
{
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    bg_main = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 4, 0);
    bg_sub = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 4, 0);
    bgSetPriority(bg_main, 3);
    bgSetPriority(bg_sub, 3);
    set_scale(false, 256);
    set_scale(true, 256);

    target.bitmap[0] = (uint32_t *)bgGetGfxPtr(bg_main);
    target.bitmap[1] = (uint32_t *)bgGetGfxPtr(bg_sub);
    target.palette[0] = BG_PALETTE;
    target.palette[1] = NULL;

    init_info_console();
    init_status_console();
    video_clear();
    video_menu();
}

void video_clear(void)
{
    // DMA fill from a value, no CPU buffer involved
    dmaFillWords(0, target.bitmap[0], 256 * 256);
    dmaFillWords(0, target.bitmap[1], 256 * 256);
}

void video_menu(void)
{
    // the stream overwrote the palettes: load the fonts again
    init_info_console();
    init_status_console();
    bgHide(bg_main);
    bgHide(bg_sub);
    bgShow(con_info.bgId);
    bgShow(con_status.bgId);
    lcdMainOnTop();
    consoleSelect(&con_status);
}

void video_stream(uint8_t mode, bool on_top)
{
    set_scale(false, mode == MODE_128x96 ? 128 : 256);
    set_scale(true, 256);
    bgShow(bg_main);
    bgHide(con_info.bgId);
    if (mode == MODE_256x384)
    {
        // top half on the top screen (main), bottom half below (sub); no status
        // console, it would need the same palette
        target.palette[1] = BG_PALETTE_SUB;
        bgShow(bg_sub);
        bgHide(con_status.bgId);
        lcdMainOnTop();
    }
    else
    {
        // picture on one screen, status on the other
        // (the sub palette may still hold a two-screen picture: reload the font)
        target.palette[1] = NULL;
        bgHide(bg_sub);
        init_status_console();
        bgShow(con_status.bgId);
        if (on_top)
            lcdMainOnTop();
        else
            lcdMainOnBottom();
    }
    consoleSelect(&con_status);
}

const DecodeTarget *video_target(void)
{
    return &target;
}

#include "common.h"
#include <string.h>
#include <libtwl/dma/dmaNitro.h>
#include <nds/arm9/cache.h>
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "gui/palette/DirectPalette.h"
#include "core/math/ColorConverter.h"
#include "sharedMemory.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "services/settings/AppSettings.h"
#include "romBrowser/Theme/dsrunner/DsRunnerLibrary.h"
#include "StatusBarView.h"

// 3x5 compact bitmap font for digits 0-9 and colon
static const u8 sDigitFont[11][5] = {
    { 0b111, 0b101, 0b101, 0b101, 0b111 }, // '0'
    { 0b010, 0b110, 0b010, 0b010, 0b111 }, // '1'
    { 0b111, 0b001, 0b111, 0b100, 0b111 }, // '2'
    { 0b111, 0b001, 0b111, 0b001, 0b111 }, // '3'
    { 0b101, 0b101, 0b111, 0b001, 0b001 }, // '4'
    { 0b111, 0b100, 0b111, 0b001, 0b111 }, // '5'
    { 0b111, 0b100, 0b111, 0b101, 0b111 }, // '6'
    { 0b111, 0b001, 0b010, 0b010, 0b010 }, // '7'
    { 0b111, 0b101, 0b111, 0b101, 0b111 }, // '8'
    { 0b111, 0b101, 0b111, 0b001, 0b111 }, // '9'
    { 0b000, 0b010, 0b000, 0b010, 0b000 }  // ':'
};

StatusBarView::StatusBarView()
{
    SetPosition(256 - 66, 5);
    memset(_tileBuffer, 0, sizeof(_tileBuffer));
}

void StatusBarView::InitVram(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _vramOffset = objVramManager->Alloc(sizeof(_tileBuffer));
        _vramAddress = objVramManager->GetVramAddress(_vramOffset);
        _needsUpload = true;
    }
}

void StatusBarView::PutPixel(int x, int y, u8 color)
{
    if (x < 0 || x >= 64 || y < 0 || y >= 16) return;

    int spriteIdx = (x >= 32) ? 1 : 0;
    int sx = x % 32;
    int sy = y;

    int tileX = sx / 8;
    int tileY = sy / 8;
    int tileIdx = spriteIdx * 8 + (tileY * 4 + tileX);

    int px = sx % 8;
    int py = sy % 8;
    int byteOffset = tileIdx * 32 + (py * 4) + (px / 2);

    if (px & 1)
        _tileBuffer[byteOffset] = (_tileBuffer[byteOffset] & 0x0F) | ((color & 0xF) << 4);
    else
        _tileBuffer[byteOffset] = (_tileBuffer[byteOffset] & 0xF0) | (color & 0xF);
}

void StatusBarView::DrawChar(int x, int y, char c, u8 color)
{
    int glyph = -1;
    if (c >= '0' && c <= '9')
        glyph = c - '0';
    else if (c == ':')
        glyph = 10;

    if (glyph < 0) return;

    for (int row = 0; row < 5; row++)
    {
        u8 bits = sDigitFont[glyph][row];
        for (int col = 0; col < 3; col++)
        {
            if (bits & (1 << (2 - col)))
            {
                PutPixel(x + col, y + row, color);
            }
        }
    }
}

void StatusBarView::RedrawBitmap(int hours, int minutes, u16 batteryState)
{
    memset(_tileBuffer, 0, sizeof(_tileBuffer));

    // Pill background: width 60, height 14 (radius 6). Without the clock only the right part
    // around the battery is left.
    int pillStart = _showClock ? 0 : 30;
    for (int y = 0; y < 14; y++)
    {
        int inset = 0;
        if (y == 0 || y == 13) inset = 4;
        else if (y == 1 || y == 12) inset = 2;
        else if (y == 2 || y == 11) inset = 1;

        for (int x = pillStart + inset; x < 60 - inset; x++)
        {
            PutPixel(x, y, 1); // color 1 = Pill background
        }
    }

    if (_showClock)
    {
        // Time text at x=6, y=4: "HH:MM"
        int h1 = hours / 10;
        int h2 = hours % 10;
        int m1 = minutes / 10;
        int m2 = minutes % 10;

        DrawChar(6,  4, '0' + h1, 3);
        DrawChar(11, 4, '0' + h2, 3);
        DrawChar(16, 4, ':',      3);
        DrawChar(20, 4, '0' + m1, 3);
        DrawChar(25, 4, '0' + m2, 3);
    }

    // Battery icon at x=36..52, y=3..10
    // Outline: (36, 3) to (49, 10)
    for (int x = 36; x <= 49; x++)
    {
        PutPixel(x, 3, 3);
        PutPixel(x, 10, 3);
    }
    for (int y = 3; y <= 10; y++)
    {
        PutPixel(36, y, 3);
        PutPixel(49, y, 3);
    }
    // Terminal tip at (50, 5) to (51, 8)
    for (int y = 5; y <= 8; y++)
    {
        PutPixel(50, y, 3);
        PutPixel(51, y, 3);
    }

    // Fill inside battery: (38, 5) to (47, 8).
    // The DS (Lite) can only report "battery low" or "ok", so this is no level indicator:
    // the bar is full while the battery is fine and nearly empty and red once it is low.
    u8 fillColor = 4; // Green (ok)
    int fillEnd = 47;
    if (batteryState & 1)
    {
        fillColor = 5; // Red (low)
        fillEnd = 39;
    }
    else if (batteryState & 2)
    {
        fillColor = 6; // Yellow (external power connected)
    }

    for (int y = 5; y <= 8; y++)
    {
        for (int x = 38; x <= fillEnd; x++)
        {
            PutPixel(x, y, fillColor);
        }
    }

    _needsUpload = true;
}

void StatusBarView::Update()
{
    View::Update();

    // the clock can be switched off in the display settings (settings.json "showClock")
    const auto* appSettings = gDsRunnerLibrary.appSettings;
    bool showClock = !appSettings || appSettings->showClock;
    bool showClockChanged = showClock != _showClock;
    _showClock = showClock;

    // The rtc is read through an ipc request to the arm7 that blocks the caller, so only
    // ask for it about once a second. The minute display can't change faster than that.
    if (!showClockChanged && _lastHour >= 0 && ++_framesSinceRtcRead < 60)
    {
        return;
    }
    _framesSinceRtcRead = 0;

    int hour = 0;
    int minute = 0;
    if (showClock)
    {
        rtc_datetime_t dt;
        rtc_readDateTime(&dt);

        // BCD to decimal conversion
        hour = (dt.time.hour >> 4) * 10 + (dt.time.hour & 0x0F);
        minute = (dt.time.minute >> 4) * 10 + (dt.time.minute & 0x0F);
    }
    u16 battery = SHARED_BATTERY;

    if (showClockChanged || hour != _lastHour || minute != _lastMinute || battery != _lastBattery)
    {
        _lastHour = hour;
        _lastMinute = minute;
        _lastBattery = battery;
        RedrawBitmap(hour, minute, battery);
    }
}

void StatusBarView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
        return;

    u16 palette[16] = { 0 };
    palette[0] = 0; // transparent
    palette[1] = ColorConverter::ToGBGR565(DsRunnerColors::Pill);
    palette[2] = ColorConverter::ToGBGR565(DsRunnerColors::Pill);
    palette[3] = ColorConverter::ToGBGR565(DsRunnerColors::Text);
    palette[4] = ColorConverter::ToGBGR565(DsRunnerColors::BatteryGood);
    palette[5] = ColorConverter::ToGBGR565(DsRunnerColors::BatteryLow);
    palette[6] = ColorConverter::ToGBGR565(Rgb<8, 8, 8>(255, 230, 80));

    u32 palRow = graphicsContext.GetPaletteManager().AllocRow(
        DirectPalette(palette), _position.y, _position.y + 16);

    // Two 32x16 OAM sprites side by side
    auto oams = graphicsContext.GetOamManager().AllocOams(2);
    OamBuilder::OamWithSize<32, 16>(_position.x, _position.y, _vramOffset >> 7)
        .WithPalette16(palRow)
        .WithPriority(0)
        .Build(oams[0]);

    OamBuilder::OamWithSize<32, 16>(_position.x + 32, _position.y, (_vramOffset + 256) >> 7)
        .WithPalette16(palRow)
        .WithPriority(0)
        .Build(oams[1]);
}

void StatusBarView::VBlank()
{
    View::VBlank();

    if (_needsUpload && _vramAddress)
    {
        // _tileBuffer is written by the cpu, so it has to leave the data cache before dma reads it
        DC_FlushRange(_tileBuffer, sizeof(_tileBuffer));
        dma_ntrCopy32(3, _tileBuffer, (void*)_vramAddress, sizeof(_tileBuffer));
        _needsUpload = false;
    }
}

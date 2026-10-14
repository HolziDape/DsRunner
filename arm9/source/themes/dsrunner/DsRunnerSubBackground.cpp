#include "common.h"
#include <nds/arm9/background.h>
#include <nds/arm9/cache.h>
#include <libtwl/dma/dmaNitro.h>
#include <libtwl/mem/memVram.h>
#include "core/math/RgbMixer.h"
#include "core/math/ColorConverter.h"
#include "DsRunnerColorScheme.h"
#include "DsRunnerHeroState.h"
#include "DsRunnerSubBackground.h"

// The gradient only depends on the screen line, so the background is built from 24 tiles
// (one per 8 lines, every tile line a different palette index) that repeat across the screen.
// Selecting another console then only means rewriting the 192 palette entries.
#define GRADIENT_LINES      192
// lines from this one on are flat DsRunnerColors::BgTop: the area for the info text
#define GRADIENT_BAND_START 118
#define TILE_ROWS           (GRADIENT_LINES / 8)
#define EXT_PALETTE_BG0     0x06898000

static void uploadGradientPalette(const DsRunnerConsoles::Color& hero)
{
    mem_setVramHMapping(MEM_VRAM_H_LCDC);
    vu16* palette = (vu16*)EXT_PALETTE_BG0;
    for (int y = 0; y < GRADIENT_LINES; y++)
    {
        DsRunnerConsoles::Color color = DsRunnerColors::BgTop;
        if (y < GRADIENT_BAND_START)
        {
            // ease in: the console color stays vivid at the top and fades out towards the text band
            int t = y * 256 / GRADIENT_BAND_START;
            int eased = (t * t) >> 8;
            color = RgbMixer::Lerp(hero, DsRunnerColors::BgTop, eased, 256);
        }
        palette[1 + y] = ColorConverter::ToGBGR565(color);
    }
    mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
}

DsRunnerSubBackground::DsRunnerSubBackground(const MaterialColorScheme* colorScheme, DsRunnerHeroState* heroState)
    : _colorScheme(colorScheme), _heroState(heroState), _art(std::make_unique<DsRunnerTopArt>()) { }

void DsRunnerSubBackground::Update()
{
    if (_heroState->ConsumeArtDirty())
    {
        _art->Request(_heroState->GetArtPath(), _heroState->GetArtFallbackPath(), _heroState->GetArtTint(),
            _heroState->GetIoTaskQueue());
    }
    _art->Update();
}

void DsRunnerSubBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    uploadGradientPalette(_heroState->GetColor());
    _heroState->ConsumeDirty();

    alignas(32) static u8 tiles[TILE_ROWS * 64];
    alignas(32) static u16 map[32 * 32];
    for (int row = 0; row < TILE_ROWS; row++)
    {
        for (int line = 0; line < 8; line++)
        {
            // palette index 0 is transparent, the gradient starts at index 1
            memset(&tiles[row * 64 + line * 8], 1 + row * 8 + line, 8);
        }
    }
    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 32; x++)
        {
            map[y * 32 + x] = y < TILE_ROWS ? y : TILE_ROWS - 1;
        }
    }
    // the buffers were just written by the cpu: make them visible to dma
    DC_FlushRange(tiles, sizeof(tiles));
    DC_FlushRange(map, sizeof(map));
    dma_ntrCopy32(3, tiles, (vu8*)BG_GFX_SUB + 0x8000, sizeof(tiles));
    dma_ntrCopy32(3, map, (vu8*)BG_GFX_SUB + 0x8800, sizeof(map));
}

void DsRunnerSubBackground::VBlank()
{
    if (_heroState->ConsumeDirty())
    {
        uploadGradientPalette(_heroState->GetColor());
    }
    REG_DISPCNT_SUB |= (1 << 8);
    REG_BG0CNT_SUB = BG_32x32 | BG_PRIORITY_3 | BG_COLOR_256 | BG_MAP_BASE(17) | BG_TILE_BASE(2);
    REG_BG0HOFS_SUB = 0;
    REG_BG0VOFS_SUB = 0;

    _art->VBlank();
}

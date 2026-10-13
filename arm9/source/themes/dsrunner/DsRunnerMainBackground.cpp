#include "common.h"
#include <libtwl/mem/memVram.h>
#include "gui/Gx.h"
#include "gui/GraphicsContext.h"
#include "themes/ITheme.h"
#include "DsRunnerColorScheme.h"
#include "DsRunnerMainBackground.h"

#define BLOCK_VTX_PACK(x, y, z) (((x)&0x3FF) | ((((y) >> 3) & 0x3FF) << 10) | ((z) << 20))

DsRunnerMainBackground::DsRunnerMainBackground(const MaterialColorScheme* colorScheme)
    : _colorScheme(colorScheme) { }

void DsRunnerMainBackground::Draw(GraphicsContext& graphicsContext)
{
    Gx::MtxIdentity();
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 0);
    Gx::TexImageParam((128 * 1024) >> 3, false, false, false, false, GX_TEXSIZE_8,
        GX_TEXSIZE_8, GX_TEXFMT_PLTT16, false, GX_TEXGEN_NONE);
    // the dummy texture is solid palette color 0: make it white so the vertex colors show unchanged
    graphicsContext.GetRgb6Palette()->ApplyColor(Rgb<6, 6, 6>(Rgb<8, 8, 8>(255, 255, 255)));

    // Vertical gradient from DsRunnerColors::BgTop to DsRunnerColors::BgBottom, in 5 bit vertex colors
    constexpr auto top = Rgb<5, 5, 5>(DsRunnerColors::BgTop);
    constexpr auto bottom = Rgb<5, 5, 5>(DsRunnerColors::BgBottom);
    Gx::Begin(GX_PRIMITIVE_QUAD);

    Gx::TexCoord(0, 0);
    Gx::Color(top);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(0, 0, 500);

    Gx::Color(bottom);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(0, 192, 500);

    Gx::Color(bottom);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(256, 192, 500);

    Gx::Color(top);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(256, 0, 500);

    Gx::End();
}

void DsRunnerMainBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    mem_setVramAMapping(MEM_VRAM_AB_LCDC);
    // the 8x8 4bpp dummy texture is 32 bytes, all palette index 0
    vu32* texture = (vu32*)0x06800000;
    for (int i = 0; i < 8; i++)
        texture[i] = 0;
    mem_setVramAMapping(MEM_VRAM_AB_TEX_SLOT_1);
}

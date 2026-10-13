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

    // Vertical hardware gradient: top #0F1117 -> bottom #181C26
    // In 5-bit color:
    // BgTop (15, 17, 23) -> (1, 2, 2)
    // BgBottom (24, 28, 38) -> (3, 3, 4)
    Gx::Begin(GX_PRIMITIVE_QUAD);

    Gx::Color(1, 2, 2);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(0, 0, 500);

    Gx::Color(3, 3, 4);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(0, 192, 500);

    Gx::Color(3, 3, 4);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(256, 192, 500);

    Gx::Color(1, 2, 2);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(256, 0, 500);

    Gx::End();
}

void DsRunnerMainBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    mem_setVramAMapping(MEM_VRAM_AB_LCDC);
    // Write 0 to texture memory so the 8x8 dummy texture is solid white modulated by vertex color
    vu32* ptr = (vu32*)0x06800000;
    for (int i = 0; i < 16; i++)
        ptr[i] = 0;
    mem_setVramAMapping(MEM_VRAM_AB_TEX_SLOT_1);
}

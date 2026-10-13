#include "common.h"
#include <string.h>
#include <libtwl/dma/dmaNitro.h>
#include "gui/GraphicsContext.h"
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/font/nitroFont2.h"
#include "core/StringUtil.h"
#include "gui/palette/GradientPalette.h"
#include "themes/IFontRepository.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "DsRunnerFolderIcon.h"

#define BADGE_SIZE       32
#define BADGE_RADIUS     7
#define BADGE_BYTES      (BADGE_SIZE * BADGE_SIZE / 2)
#define LABEL_BYTES      (32 * 16 / 2)
#define LABEL_Y_OFFSET   8

// 4bpp rounded square: index 15 is solid, lower indices blend towards the background
static void buildBadge(u8* tiles)
{
    // 8x8 tiles, row by row
    memset(tiles, 0, BADGE_BYTES);
    for (int y = 0; y < BADGE_SIZE; y++)
    {
        for (int x = 0; x < BADGE_SIZE; x++)
        {
            // distance outside the rounded rectangle, measured from the nearest corner circle center
            int cx = x < BADGE_RADIUS ? BADGE_RADIUS : (x >= BADGE_SIZE - BADGE_RADIUS ? BADGE_SIZE - 1 - BADGE_RADIUS : x);
            int cy = y < BADGE_RADIUS ? BADGE_RADIUS : (y >= BADGE_SIZE - BADGE_RADIUS ? BADGE_SIZE - 1 - BADGE_RADIUS : y);
            int value = 15;
            if (cx != x || cy != y)
            {
                int dx = x - cx, dy = y - cy;
                // fixed point distance with 1/4 pixel precision, sqrt via integer search
                int d2 = (dx * dx + dy * dy) * 16;
                int dist = 0;
                while ((dist + 1) * (dist + 1) <= d2)
                    dist++;
                // coverage falls off over one pixel (4 steps) around radius + 0.5
                int edge = BADGE_RADIUS * 4 + 2;
                int cover = edge + 2 - dist; // 4 steps wide
                value = cover <= 0 ? 0 : (cover >= 4 ? 15 : cover * 15 / 4);
            }
            int tile = (y >> 3) * (BADGE_SIZE / 8) + (x >> 3);
            int offset = tile * 32 + (y & 7) * 4 + ((x & 7) >> 1);
            if (x & 1)
                tiles[offset] |= value << 4;
            else
                tiles[offset] |= value;
        }
    }
}

DsRunnerFolderIcon::DsRunnerFolderIcon(const TCHAR* name, const IFontRepository* fontRepository)
    : _fontRepository(fontRepository), _badgeColor(DsRunnerConsoles::kUnknownTile)
{
    const char* label = name;
    if (const auto* console = DsRunnerConsoles::FindByFolderName(name))
    {
        label = console->label;
        _badgeColor = console->tile;
    }
    SetLabel(label);
}

DsRunnerFolderIcon::DsRunnerFolderIcon(const char* label, const Rgb<8, 8, 8>& color, const IFontRepository* fontRepository)
    : _fontRepository(fontRepository), _badgeColor(color)
{
    SetLabel(label);
}

void DsRunnerFolderIcon::SetLabel(const char* label)
{
    // UTF-8 (the favorites badge is a star), at most 4 characters
    StringUtil::Copy(_label, label, 5);
}

void DsRunnerFolderIcon::UploadGraphics()
{
    if (_vramAddress == nullptr)
    {
        return;
    }

    // the buffers live on the stack (cached memory), so copy with the cpu instead of dma
    alignas(4) u8 badge[BADGE_BYTES];
    buildBadge(badge);
    memcpy((u8*)_vramAddress, badge, BADGE_BYTES);

    // four letter labels need the smaller font to fit in the badge
    int length = 0;
    while (_label[length] != 0)
        length++;
    auto font = _fontRepository->GetFont(length >= 4 ? FontType::Medium10 : FontType::Medium11);

    alignas(4) u8 labelTiles[LABEL_BYTES];
    memset(labelTiles, 0, sizeof(labelTiles));
    u32 textWidth, textHeight;
    nft2_measureString(font, _label, textWidth, textHeight);
    nft2_string_render_params_t renderParams;
    renderParams.x = ((int)32 - (int)textWidth) / 2;
    renderParams.y = 0;
    renderParams.width = 32;
    renderParams.height = 16;
    renderParams.a5i3 = false;
    nft2_renderString(font, _label, labelTiles, 32, &renderParams);
    memcpy((u8*)_vramAddress + BADGE_BYTES, labelTiles, sizeof(labelTiles));
}

void DsRunnerFolderIcon::Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor)
{
    auto oams = graphicsContext.GetOamManager().AllocOams(2);

    u32 badgePaletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(backgroundColor, _badgeColor), _position.y, _position.y + 32);
    OamBuilder::OamWithSize<32, 32>(_position.x, _position.y, _vramOffset >> 7)
        .WithPalette16(badgePaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oams[1]);

    u32 labelPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(_badgeColor, DsRunnerColors::White), _position.y, _position.y + 32);
    OamBuilder::OamWithSize<32, 16>(_position.x, _position.y + LABEL_Y_OFFSET, (_vramOffset + BADGE_BYTES) >> 7)
        .WithPalette16(labelPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oams[0]);
}

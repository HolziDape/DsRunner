#include "common.h"
#include <libtwl/gfx/gfx.h>
#include <libtwl/dma/dmaNitro.h>
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "dsrunnerTile.h"
#include "dsrunnerTileSelected.h"
#include "core/math/ColorConverter.h"
#include "gui/palette/DirectPalette.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "DsRunnerIconGridItemView.h"

DsRunnerIconGridItemView::DsRunnerIconGridItemView(
    std::unique_ptr<IRomBrowserItemViewModel> viewModel,
    const MaterialColorScheme* colorScheme)
    : IconGridItemView(std::move(viewModel))
    , _colorScheme(colorScheme) { }

void DsRunnerIconGridItemView::Draw(GraphicsContext& graphicsContext)
{
    // The tile sprite is 64x64 with the 40x40 body at (12, 12): drawn at position - 10 the body
    // sits 2px inside the 44x44 grid cell, leaving the 4px grid spacing for the selection glow.
    constexpr int SpriteOffset = -10;
    if (!graphicsContext.IsVisible(Rectangle(_position.x + SpriteOffset, _position.y + SpriteOffset, 64, 64)))
        return;

    bool isSelected = (_isFocused || _inputHandler.IsPenDown());

    u32 tileOffset = _bgVramOffset;
    u32 cellPaletteRow;

    if (isSelected)
    {
        // Selected: glow border + highlight
        tileOffset += dsrunnerTileTilesLen;
        u16 pal565[16];
        for (int i = 0; i < 16; i++)
        {
            pal565[i] = ColorConverter::ToGBGR565(Rgb<8, 8, 8>(ColorConverter::FromXBGR555(dsrunnerTileSelectedPal[i])));
        }
        cellPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            DirectPalette(pal565), _position.y + SpriteOffset, _position.y + SpriteOffset + 64);
    }
    else
    {
        // Unselected: dimmed card
        auto backColor = DsRunnerColors::BgTop;
        auto frontColor = DsRunnerColors::TileDim;
        u16 pal565[16];
        for (int i = 0; i < 16; i++)
        {
            auto blendFactors = ColorConverter::FromXBGR555(dsrunnerTilePal[i]);
            auto palColor = Rgb<8, 8, 8>(
                (backColor.r * blendFactors.r + frontColor.r * blendFactors.g + 16) / 31,
                (backColor.g * blendFactors.r + frontColor.g * blendFactors.g + 16) / 31,
                (backColor.b * blendFactors.r + frontColor.b * blendFactors.g + 16) / 31);
            palColor = palColor.Clamped();
            pal565[i] = ColorConverter::ToGBGR565(palColor);
        }
        cellPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            DirectPalette(pal565), _position.y + SpriteOffset, _position.y + SpriteOffset + 64);
    }

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<64, 64>(
            _position.x + SpriteOffset,
            _position.y + SpriteOffset, tileOffset >> 7)
        .WithPalette16(cellPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[0]);

    if (_icon)
    {
        _icon->SetPosition(6 + _position.x, 6 + _position.y);
        // icons sit on the tile body, so that is the color they have to blend into
        _icon->Draw(graphicsContext, isSelected ? DsRunnerColors::Tile : DsRunnerColors::TileDim);
    }
}

IconGridItemView::VramToken DsRunnerIconGridItemView::UploadGraphics(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    u32 vramOffset = 0;
    if (objVramManager)
    {
        u32 totalLen = dsrunnerTileTilesLen + dsrunnerTileSelectedTilesLen;
        vramOffset = objVramManager->Alloc(totalLen);
        dma_ntrCopy32(3, dsrunnerTileTiles, objVramManager->GetVramAddress(vramOffset), dsrunnerTileTilesLen);
        dma_ntrCopy32(3, dsrunnerTileSelectedTiles, objVramManager->GetVramAddress(vramOffset + dsrunnerTileTilesLen), dsrunnerTileSelectedTilesLen);
    }

    return IconGridItemView::VramToken(vramOffset);
}

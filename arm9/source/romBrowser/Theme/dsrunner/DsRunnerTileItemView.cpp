#include "common.h"
#include <libtwl/gfx/gfxOam.h>
#include "gui/Gx.h"
#include "gui/GraphicsContext.h"
#include "gui/OamManager.h"
#include "gui/PaletteManager.h"
#include "core/math/RgbMixer.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "DsRunnerTileItemView.h"

// corner radius of the tile body in pixels
#define CORNER_RADIUS       7
// accent ring around the selected tile
#define RING_WIDTH          2
// soft glow outside the ring
#define GLOW_WIDTH          5
#define GLOW_ALPHA          7

// depth of the layers (raw 1.3.12 z values, smaller is closer). The background is far behind (see
// DsRunnerMainBackground), the tile body is in front of its selection ring and glow.
#define Z_BODY              (-320)
#define Z_RING              (-200)
#define Z_GLOW              (-100)

// polygon ids of the two translucent glow layers: translucent polygons only blend
// over each other when their ids differ
#define POLYGON_ID_GLOW     62

using Rgb888 = Rgb<8, 8, 8>;

// Points of a quarter circle from the top (angle 0) to the side (angle 90 degrees) in steps of 22.5 degrees,
// in 1/256 of the radius: how far down from the top edge (dy) and in from the side edge (dx) they are.
static constexpr int sArcDy[] = { 0, 19, 75, 158, 256 };
static constexpr int sArcDx[] = { 256, 158, 75, 19, 0 };
static constexpr int sArcSteps = sizeof(sArcDy) / sizeof(sArcDy[0]);

static inline void vertex(int x64, int y64, int z)
{
    // x is in 1/64 pixels already; the projection has 8 times the resolution on y (see App.cpp)
    REG_GX_VTX_16 = GX_VTX_PACK(x64, y64 >> 3);
    REG_GX_VTX_16 = (u16)z;
}

/// @brief Draws a rounded rectangle as one quad strip from top to bottom with a vertical color gradient.
/// @param x, y, width, height The rectangle in pixels.
/// @param radius The corner radius in pixels.
/// @param clipTop Nothing is drawn above this screen line (the tiles scroll below the app bar).
static void drawRoundedRect(int x, int y, int width, int height, int radius,
    const Rgb888& top, const Rgb888& bottom, int z, int clipTop)
{
    if (y + height <= clipTop)
        return;

    radius = std::min(radius, std::min(width, height) / 2);
    int left = x << 6;
    int right = (x + width) << 6;
    int r64 = radius << 6;
    int top64 = y << 6;
    int height64 = height << 6;
    int clip64 = (clipTop << 6) - top64;

    Gx::Begin(GX_PRIMITIVE_QUAD_STRIP);
    int previousY = 0;
    int previousDx = 0;
    for (int i = 0; i < sArcSteps * 2; i++)
    {
        // first the arc of the top corners, then the arc of the bottom corners mirrored
        int step = i < sArcSteps ? i : (sArcSteps * 2 - 1 - i);
        int dy = (sArcDy[step] * r64) >> 8;
        int dx = (sArcDx[step] * r64) >> 8;
        int lineY = i < sArcSteps ? dy : (height64 - dy);

        if (lineY < clip64)
        {
            previousY = lineY;
            previousDx = dx;
            continue;
        }
        if (i > 0 && previousY < clip64)
        {
            // the first line below the clip line: start at the clip line, on the edge of the shape
            int clipDx = lineY == previousY ? dx
                : previousDx + (dx - previousDx) * (clip64 - previousY) / (lineY - previousY);
            Gx::Color(Rgb<5, 5, 5>(RgbMixer::Lerp(top, bottom, clip64, height64)));
            vertex(right - clipDx, top64 + clip64, z);
            vertex(left + clipDx, top64 + clip64, z);
            previousY = clip64;
        }

        Gx::Color(Rgb<5, 5, 5>(RgbMixer::Lerp(top, bottom, lineY, height64)));
        vertex(right - dx, top64 + lineY, z);
        vertex(left + dx, top64 + lineY, z);
    }
    Gx::End();
}

/// @brief Palette manager that reserves palette rows for twice the line range, around a center line.
///        Icons allocate their palettes for their unscaled size, scaled 2x they cover twice the lines.
class DoubleHeightPaletteManager : public PaletteManager
{
public:
    DoubleHeightPaletteManager(PaletteManager& paletteManager, int centerY)
        : _paletteManager(paletteManager), _centerY(centerY) { }

    u32 AllocRow(const IPalette& palette, int yStart, int yEnd) override
    {
        return _paletteManager.AllocRow(palette,
            _centerY + (yStart - _centerY) * 2, _centerY + (yEnd - _centerY) * 2);
    }

private:
    PaletteManager& _paletteManager;
    int _centerY;
};

static void getObjSize(const gfx_oam_entry_t& oam, int& width, int& height)
{
    static constexpr u8 sSizes[3][4][2] =
    {
        { { 8, 8 }, { 16, 16 }, { 32, 32 }, { 64, 64 } }, // square
        { { 16, 8 }, { 32, 8 }, { 32, 16 }, { 64, 32 } }, // wide
        { { 8, 16 }, { 8, 32 }, { 16, 32 }, { 32, 64 } }  // tall
    };
    u32 shape = std::min<u32>(oam.attr0 >> 14, 2);
    u32 size = oam.attr1 >> 14;
    width = sSizes[shape][size][0];
    height = sSizes[shape][size][1];
}

DsRunnerTileItemView::DsRunnerTileItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel,
    int width, int height, int iconScale, int clipTop)
    : IconGridItemView(std::move(viewModel)), _width(width), _height(height), _iconScale(iconScale)
    , _clipTop(clipTop) { }

void DsRunnerTileItemView::Draw(GraphicsContext& graphicsContext)
{
    constexpr int margin = RING_WIDTH + GLOW_WIDTH;
    if (_position.y + _height <= _clipTop ||
        !graphicsContext.IsVisible(Rectangle(_position.x - margin, _position.y - margin,
            _width + 2 * margin, _height + 2 * margin)))
    {
        return;
    }

    bool isSelected = _isFocused || _inputHandler.IsPenDown();

    Rgb888 top, bottom;
    if (_hasTint)
    {
        // console tiles: the console badge (icon) sits on a frame that is lighter than the badge
        // when selected and sunk into the background otherwise
        if (isSelected)
        {
            top = RgbMixer::Lerp(_tint, DsRunnerColors::White, 3, 8);
            bottom = RgbMixer::Lerp(_tint, DsRunnerColors::White, 2, 8);
        }
        else
        {
            top = RgbMixer::Lerp(_tint, DsRunnerColors::BgTop, 3, 8);
            bottom = RgbMixer::Lerp(_tint, DsRunnerColors::BgTop, 5, 8);
        }
    }
    else if (isSelected)
    {
        top = Rgb888(58, 66, 90);
        bottom = Rgb888(40, 46, 64);
    }
    else
    {
        top = Rgb888(33, 38, 53);
        bottom = DsRunnerColors::TileDim;
    }

    Gx::MtxIdentity();
    // no texture: the polygons show their vertex colors
    Gx::TexImageParam(0, false, false, false, false, GX_TEXSIZE_8, GX_TEXSIZE_8,
        GX_TEXFMT_NONE, false, GX_TEXGEN_NONE);
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_BOTH,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, graphicsContext.GetPolygonId());
    drawRoundedRect(_position.x, _position.y, _width, _height, CORNER_RADIUS, top, bottom, Z_BODY, _clipTop);

    if (isSelected)
    {
        drawRoundedRect(_position.x - RING_WIDTH, _position.y - RING_WIDTH,
            _width + 2 * RING_WIDTH, _height + 2 * RING_WIDTH, CORNER_RADIUS + RING_WIDTH,
            DsRunnerColors::Accent, DsRunnerColors::Accent, Z_RING, _clipTop - margin);

        // The ring and glow may reach into the app bar: only the selected tile has them, and it is
        // always scrolled fully into view. Translucent polygons are drawn after the opaque ones,
        // so the glow only shows outside the ring.
        Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_BOTH,
            false, false, false, GX_DEPTH_FUNC_LESS, false, GLOW_ALPHA, POLYGON_ID_GLOW);
        drawRoundedRect(_position.x - margin, _position.y - margin,
            _width + 2 * margin, _height + 2 * margin, CORNER_RADIUS + margin,
            DsRunnerColors::Accent, DsRunnerColors::Accent, Z_GLOW, _clipTop - margin);
        Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_BOTH,
            false, false, false, GX_DEPTH_FUNC_LESS, false, GLOW_ALPHA, POLYGON_ID_GLOW + 1);
        drawRoundedRect(_position.x - RING_WIDTH - 2, _position.y - RING_WIDTH - 2,
            _width + 2 * RING_WIDTH + 4, _height + 2 * RING_WIDTH + 4, CORNER_RADIUS + RING_WIDTH + 2,
            DsRunnerColors::Accent, DsRunnerColors::Accent, Z_GLOW - 10, _clipTop - margin);
    }

    // sprites can't be cut off: the icon of a tile that scrolls under the app bar disappears
    // as soon as it would reach into it
    int iconHalfSize = 16 * _iconScale;
    if (_icon && _position.y + _height / 2 - iconHalfSize >= _clipTop)
    {
        // the icon blends into the middle of the body gradient
        DrawIcon(graphicsContext, RgbMixer::Lerp(top, bottom, 1, 2));
    }
}

void DsRunnerTileItemView::DrawIcon(GraphicsContext& graphicsContext, const Rgb888& bodyColor)
{
    // icons are 32x32, drawn centered on the tile
    int centerX = _position.x + _width / 2;
    int centerY = _position.y + _height / 2;
    _icon->SetPosition(centerX - 16, centerY - 16);

    if (_iconScale != 2 ||
        !graphicsContext.IsVisible(Rectangle(centerX - 32, centerY - 32, 64, 64)))
    {
        _icon->Draw(graphicsContext, bodyColor);
        return;
    }

    // Scaled 2x: let the icon build its oams as usual (with palette rows reserved for the doubled
    // height), then turn them into double size affine sprites scaled around the icon center.
    auto& oamManager = graphicsContext.GetOamManager();
    gfx_oam_entry_t* oamsEnd = oamManager.AllocOams(0);
    {
        DoubleHeightPaletteManager paletteManager(graphicsContext.GetPaletteManager(), centerY);
        GraphicsContext scaledContext(&oamManager, &paletteManager, graphicsContext.GetRgb6Palette());
        scaledContext.SetPriority(graphicsContext.GetPriority());
        scaledContext.SetPolygonId(graphicsContext.GetPolygonId());
        _icon->Draw(scaledContext, bodyColor);
    }
    gfx_oam_entry_t* oamsStart = oamManager.AllocOams(0);

    // one matrix per flip combination (animated nds icons can flip), allocated on demand
    int matrixIds[4] = { -1, -1, -1, -1 };
    for (gfx_oam_entry_t* oam = oamsStart; oam < oamsEnd; oam++)
    {
        if (oam->attr0 & GFX_OAM_ATTR0_HIDDEN)
            continue;

        u32 flip = (oam->attr1 >> 12) & 3;
        if (matrixIds[flip] < 0)
        {
            u32 matrixId;
            gfx_oam_mtx_t* matrix = oamManager.AllocMatrices(1, matrixId);
            if (matrixId >= 32)
            {
                // out of affine matrices: leave the remaining sprites unscaled
                break;
            }
            // the matrix maps screen pixels to sprite pixels: half a pixel per pixel is 2x
            matrix->pa = (flip & 1) ? -0x80 : 0x80;
            matrix->pb = 0;
            matrix->pc = 0;
            matrix->pd = (flip & 2) ? -0x80 : 0x80;
            matrixIds[flip] = matrixId;
        }

        int width, height;
        getObjSize(*oam, width, height);
        int x = oam->attr1 & 0x1FF;
        if (x >= 256)
            x -= 512;
        int y = oam->attr0 & 0xFF;
        if (y >= 192)
            y -= 256;

        // scale the sprite center away from the icon center, the double size box is twice the sprite size
        int newX = centerX + (x + width / 2 - centerX) * 2 - width;
        int newY = centerY + (y + height / 2 - centerY) * 2 - height;

        oam->attr0 = (oam->attr0 & ~0x3FF) | GFX_OAM_ATTR0_DOUBLE_AFFINE | (newY & 0xFF);
        oam->attr1 = (oam->attr1 & ~0x3FFF) | (matrixIds[flip] << 9) | (newX & 0x1FF);
    }
}

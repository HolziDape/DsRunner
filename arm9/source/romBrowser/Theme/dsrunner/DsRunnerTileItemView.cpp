#include "common.h"
#include <libtwl/gfx/gfxOam.h>
#include "gui/Gx.h"
#include "gui/GraphicsContext.h"
#include "gui/OamManager.h"
#include "gui/PaletteManager.h"
#include "core/math/RgbMixer.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "romBrowser/IRomBrowserController.h"
#include "romBrowser/FileInfo.h"
#include <nds/arm9/cache.h>
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "fat/File.h"
#include "DsRunnerLibrary.h"
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
// the tile image is in front of the body, the favorite star in front of both
#define Z_IMAGE             (-330)
#define Z_STAR              (-340)
// tile image size in the 4x3 grid: the tile height minus a small margin (64 px in the 3x2 grid)
#define IMAGE_MARGIN        3

// favorite star: outer radius in the 3x2 / 4x3 grid and distance of its center to the top right
// corner. It sits on the rounded corner like a badge, the large icons of the 3x2 grid leave no room
// inside the tile (sprites are drawn over the 3D layer, so the icon may cover a few pixels of it).
#define STAR_RADIUS_LARGE   6
#define STAR_RADIUS_SMALL   5
#define STAR_INSET          5

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

// Points of a five-pointed star, alternating outer (radius 256) and inner (radius 98) points,
// clockwise from the top, in 1/256 of the outer radius.
static constexpr s16 sStarX[] = { 0, 58, 243, 93, 150, 0, -150, -93, -243, -58 };
static constexpr s16 sStarY[] = { -256, -79, -79, 30, 207, 98, 207, 30, -79, -79 };
static constexpr int sStarPoints = sizeof(sStarX) / sizeof(sStarX[0]);

/// @brief Draws a filled star as triangles around its center.
/// @param centerX, centerY The center in pixels.
/// @param radius The outer radius in pixels.
static void drawStar(int centerX, int centerY, int radius, const Rgb888& color, int z)
{
    int cx64 = centerX << 6;
    int cy64 = centerY << 6;
    Gx::Begin(GX_PRIMITIVE_TRIANGLE);
    Gx::Color(Rgb<5, 5, 5>(color));
    for (int i = 0; i < sStarPoints; i++)
    {
        int next = i + 1 < sStarPoints ? i + 1 : 0;
        vertex(cx64, cy64, z);
        vertex(cx64 + ((sStarX[i] * radius) >> 2), cy64 + ((sStarY[i] * radius) >> 2), z);
        vertex(cx64 + ((sStarX[next] * radius) >> 2), cy64 + ((sStarY[next] * radius) >> 2), z);
    }
    Gx::End();
}

DsRunnerTileItemView::DsRunnerTileItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel,
    IRomBrowserController* romBrowserController, VBlankTextureLoader* textureLoader,
    int width, int height, int iconScale, int clipTop)
    : IconGridItemView(std::move(viewModel)), _width(width), _height(height), _iconScale(iconScale)
    , _clipTop(clipTop), _romBrowserController(romBrowserController), _textureLoader(textureLoader) { }

DsRunnerTileItemView::~DsRunnerTileItemView()
{
    if (_textureLoader)
        _textureLoader->CancelLoad(_textureLoadRequest);
}

void DsRunnerTileItemView::InitVram(const VramContext& vramContext)
{
    IconGridItemView::InitVram(vramContext);
    const auto texVramManager = vramContext.GetTexVramManager();
    const auto texPlttVramManager = vramContext.GetTexPlttVramManager();
    if (texVramManager && texPlttVramManager && _textureLoader)
    {
        _texVramOffset = texVramManager->Alloc(sizeof(_tileTexels));
        _plttVramOffset = texPlttVramManager->Alloc(sizeof(_tilePalette));
        _hasTextureVram = true;
    }
}

void DsRunnerTileItemView::ClearTileImage()
{
    _bindNumber = _bindNumber + 1;
    _tileImageLoaded = false;
    if (_tileImageRequested)
    {
        _textureLoader->CancelLoad(_textureLoadRequest);
        _tileImageRequested = false;
    }
}

static inline u32 readLe32(const u8* data)
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

bool DsRunnerTileItemView::LoadTileImage(const char* path, const vu8& cancelRequested)
{
    if (!_hasTextureVram || cancelRequested)
        return false;
    u32 bindNumber = _bindNumber;

    auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return false;

    // 64x64, 8 bits per pixel, uncompressed (see "Kachel-Bilder" in docs/CONFIG.md)
    alignas(4) u8 header[54];
    if (!file->ReadExact(header, sizeof(header)) || header[0] != 'B' || header[1] != 'M')
        return false;
    u32 pixelOffset = readLe32(&header[0x0A]);
    u32 dibSize = readLe32(&header[0x0E]);
    s32 width = (s32)readLe32(&header[0x12]);
    s32 height = (s32)readLe32(&header[0x16]);
    u32 bpp = header[0x1C] | (header[0x1D] << 8);
    u32 colors = readLe32(&header[0x2E]);
    if (colors == 0 || colors > 256)
        colors = 256;
    if (dibSize < 40 || width != DSRUNNER_TILE_IMAGE_SIZE || bpp != 8 || readLe32(&header[0x1E]) != 0 ||
        (height != DSRUNNER_TILE_IMAGE_SIZE && height != -DSRUNNER_TILE_IMAGE_SIZE))
    {
        return false;
    }

    // only the io thread loads tile images, so one buffer for the BMP palette is enough
    alignas(4) static u8 sBmpPalette[256 * 4];
    if (file->Seek(14 + dibSize) != FR_OK || !file->ReadExact(sBmpPalette, colors * 4) ||
        file->Seek(pixelOffset) != FR_OK || !file->ReadExact(_tileTexels, sizeof(_tileTexels)))
    {
        return false;
    }
    if (cancelRequested || bindNumber != _bindNumber)
        return false;

    if (height > 0)
    {
        // bottom-up: textures go from the top row down
        for (int y = 0; y < DSRUNNER_TILE_IMAGE_SIZE / 2; y++)
        {
            u32* top = (u32*)&_tileTexels[y * DSRUNNER_TILE_IMAGE_SIZE];
            u32* bottom = (u32*)&_tileTexels[(DSRUNNER_TILE_IMAGE_SIZE - 1 - y) * DSRUNNER_TILE_IMAGE_SIZE];
            for (int x = 0; x < DSRUNNER_TILE_IMAGE_SIZE / 4; x++)
                std::swap(top[x], bottom[x]);
        }
    }
    for (u32 i = 0; i < 256; i++)
    {
        const u8* bgra = &sBmpPalette[i * 4];
        _tilePalette[i] = i < colors ? ((bgra[2] >> 3) | ((bgra[1] >> 3) << 5) | ((bgra[0] >> 3) << 10)) : 0;
    }
    // the vblank texture loader copies the buffers with dma
    DC_FlushRange(_tileTexels, sizeof(_tileTexels));
    DC_FlushRange(_tilePalette, sizeof(_tilePalette));

    if (cancelRequested || bindNumber != _bindNumber)
        return false;
    _tileImageLoaded = true;
    return true;
}

void DsRunnerTileItemView::Update()
{
    IconGridItemView::Update();
    if (_tileImageLoaded && !_tileImageRequested)
    {
        _textureLoadRequest = VBlankTextureLoadRequest(_tileTexels, sizeof(_tileTexels), _texVramOffset,
            _tilePalette, sizeof(_tilePalette), _plttVramOffset, nullptr, nullptr);
        _textureLoader->RequestLoad(_textureLoadRequest);
        _tileImageRequested = true;
    }
}

void DsRunnerTileItemView::DrawTileImage()
{
    // the image sits in the middle of the tile, 1:1 in the 3x2 grid (the 4x3 grid shows no tile images)
    int size = std::min(DSRUNNER_TILE_IMAGE_SIZE, std::min(_width, _height) - 2 * IMAGE_MARGIN);
    int x0 = _position.x + (_width - size) / 2;
    int y0 = _position.y + (_height - size) / 2;
    int x1 = x0 + size;
    int y1 = y0 + size;
    if (y1 <= _clipTop)
        return;
    // cut off under the app bar like the tile body: move the top edge and its texture row down
    int texTop64 = 0;
    int top64 = y0 << 6;
    if (y0 < _clipTop)
    {
        texTop64 = (_clipTop - y0) * DSRUNNER_TILE_IMAGE_SIZE * 64 / size;
        top64 = _clipTop << 6;
    }

    Gx::TexImageParam(_texVramOffset >> 3, false, false, false, false, GX_TEXSIZE_64, GX_TEXSIZE_64,
        GX_TEXFMT_PLTT256, false, GX_TEXGEN_NONE);
    Gx::TexPlttBase(_plttVramOffset >> 4);
    Gx::Color(0x7FFF);
    auto texTop = fix16<4>::FromRawValue(texTop64 >> 2);
    auto texBottom = fix16<4>(DSRUNNER_TILE_IMAGE_SIZE);
    auto texRight = fix16<4>(DSRUNNER_TILE_IMAGE_SIZE);
    Gx::Begin(GX_PRIMITIVE_QUAD);
    Gx::TexCoord(0, texTop);
    vertex(x0 << 6, top64, Z_IMAGE);
    Gx::TexCoord(0, texBottom);
    vertex(x0 << 6, y1 << 6, Z_IMAGE);
    Gx::TexCoord(texRight, texBottom);
    vertex(x1 << 6, y1 << 6, Z_IMAGE);
    Gx::TexCoord(texRight, texTop);
    vertex(x1 << 6, top64, Z_IMAGE);
    Gx::End();

    // back to untextured polygons for the rest of the tile
    Gx::TexImageParam(0, false, false, false, false, GX_TEXSIZE_8, GX_TEXSIZE_8,
        GX_TEXFMT_NONE, false, GX_TEXGEN_NONE);
}

bool DsRunnerTileItemView::IsFavorite()
{
    const auto& favorites = gDsRunnerLibrary.favorites;
    if (_favoriteVersion != favorites.GetVersion())
    {
        _favoriteVersion = favorites.GetVersion();
        _isFavorite = _item && favorites.GetCount() > 0 &&
            _item->GetFileType()->GetClassification() != FileTypeClassification::Folder &&
            favorites.Contains(_romBrowserController->GetItemFolderPath(*_item), _item->GetFileName());
    }
    return _isFavorite;
}

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

    bool showTileImage = IsTileImageReady();
    if (showTileImage)
        DrawTileImage();

    // favorites get a small star in the top right corner
    int starRadius = _iconScale == 2 ? STAR_RADIUS_LARGE : STAR_RADIUS_SMALL;
    int starX = _position.x + _width - STAR_INSET;
    int starY = _position.y + STAR_INSET;
    if (starY - starRadius >= _clipTop && IsFavorite())
    {
        drawStar(starX, starY, starRadius, DsRunnerColors::Favorite, Z_STAR);
    }

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
    if (!showTileImage && _icon && _position.y + _height / 2 - iconHalfSize >= _clipTop)
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

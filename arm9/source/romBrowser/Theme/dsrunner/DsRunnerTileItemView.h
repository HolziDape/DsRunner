#pragma once
#include "core/math/Rgb.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "gui/VBlankTextureLoader.h"
#include "romBrowser/views/IconGridItemView.h"

class IRomBrowserController;
class FileInfo;

#define DSRUNNER_TILE_IMAGE_SIZE    64

/// @brief Large tile of the DsRunner tile grids (3x2 and 4x3).
///        The tile body is drawn with the 3D engine (any size, rounded corners, glow when selected),
///        the file icon is an OBJ on top of it and can be scaled up (2x in the 3x2 grid).
///        In the 3x2 grid games with a tile image (/_dsrunner/tiles, 64x64 8 bpp, made by DsRunner Art)
///        show it as a texture instead of the icon. The 4x3 grid passes no texture loader: no texture
///        vram, no tile images, only the icon 1:1.
///        Folders show the built-in image of their console (DsRunnerConsoleArt) with their label below:
///        the 64x64 template in the 3x2 grid, the 32x32 icon in the 4x3 grid.
class DsRunnerTileItemView : public IconGridItemView
{
    SHARED_ONLY(DsRunnerTileItemView)

public:
    ~DsRunnerTileItemView() override;

    void InitVram(const VramContext& vramContext) override;
    void Update() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, _width, _height);
    }

    void Draw(GraphicsContext& graphicsContext) override;

    /// @brief Colors the tile body, e.g. in the color of a console folder.
    void SetTint(const Rgb<8, 8, 8>& color)
    {
        _tint = color;
        _hasTint = true;
    }

    /// @brief Back to the neutral tile color.
    void ClearTint()
    {
        _hasTint = false;
    }

    /// @brief Sets the file the tile shows, for the favorite marker.
    void SetItem(const FileInfo* item)
    {
        _item = item;
        // look the favorite state up again on the next draw
        _favoriteVersion = ~0u;
        _label[0] = 0;
        _labelRendered = false;
        ClearTileImage();
    }

    /// @brief Sets the label shown under the console image of a folder (after SetItem, main thread).
    ///        It is drawn once RenderLabel() has run and the folder shows a console image.
    void SetLabel(const char* label);

    /// @brief Renders the label into vram. Called on the io thread while binding.
    void RenderLabel();

    /// @brief Loads the tile image of the bound file. Called on the io thread while binding.
    /// @param tint Null for the user's own images. For a template of DsRunnerConsoleArt the color it is
    ///        tinted with: then color 0 is transparent and the label is shown below the image.
    /// @return True when the image was loaded (it is shown once it reached vram).
    bool LoadTileImage(const char* path, const DsRunnerConsoles::Color* tint, const vu8& cancelRequested);

private:
    int _width;
    int _height;
    int _iconScale;
    int _clipTop;
    Rgb<8, 8, 8> _tint;
    bool _hasTint = false;
    IRomBrowserController* _romBrowserController;
    const FileInfo* _item = nullptr;
    // the favorite state is cached until the favorites change (see DsRunnerFavorites::GetVersion)
    u32 _favoriteVersion = ~0u;
    bool _isFavorite = false;

    // tile image: loaded into the buffers on the io thread, then copied to vram during vblank
    VBlankTextureLoader* _textureLoader;
    u32 _texVramOffset = 0;
    u32 _plttVramOffset = 0;
    bool _hasTextureVram = false;
    // changes with every bind, so a load for an earlier item is dropped
    volatile u32 _bindNumber = 0;
    volatile bool _tileImageLoaded = false;
    bool _tileImageRequested = false;
    VBlankTextureLoadRequest _textureLoadRequest;
    alignas(32) u8 _tileTexels[DSRUNNER_TILE_IMAGE_SIZE * DSRUNNER_TILE_IMAGE_SIZE];
    alignas(32) u16 _tilePalette[256];
    // the loaded tile image is a console template (transparent, with the label below it)
    volatile bool _tileImageIsConsoleArt = false;

    // label under the console image: 64x16 pixels (two 32x16 sprites), only with DsRunnerConsoleArt
    char _label[32] = { 0 };
    volatile bool _labelRendered = false;
    vu16* _labelVram = nullptr;
    u32 _labelVramOffset = 0;

    DsRunnerTileItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel, IRomBrowserController* romBrowserController,
        VBlankTextureLoader* textureLoader, int width, int height, int iconScale, int clipTop);

    void ClearTileImage();
    bool IsTileImageReady() const
    {
        return _tileImageRequested && _textureLoadRequest.GetState() == VBlankTextureLoadRequestState::LoadComplete;
    }
    void DrawTileImage();

    void DrawIcon(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& bodyColor, int yOffset);
    void DrawLabel(GraphicsContext& graphicsContext, int y, const Rgb<8, 8, 8>& bodyColor);
    bool IsFavorite();
};

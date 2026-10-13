#pragma once
#include "core/math/Rgb.h"
#include "romBrowser/views/IconGridItemView.h"

class IRomBrowserController;
class FileInfo;

/// @brief Large tile of the DsRunner tile grids (3x2 and 4x3).
///        The tile body is drawn with the 3D engine (any size, rounded corners, glow when selected),
///        the file icon is an OBJ on top of it and can be scaled up (2x in the 3x2 grid).
class DsRunnerTileItemView : public IconGridItemView
{
    SHARED_ONLY(DsRunnerTileItemView)

public:
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
    }

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

    DsRunnerTileItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel, IRomBrowserController* romBrowserController,
        int width, int height, int iconScale, int clipTop);

    void DrawIcon(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& bodyColor);
    bool IsFavorite();
};

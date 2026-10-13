#pragma once
#include "gui/views/RecyclerView.h"
#include "romBrowser/DisplayMode/RomBrowserDisplayMode.h"
#include "DsRunnerTileGridFileRecyclerAdapter.h"

/// @brief DsRunner tile grid: a fixed number of large tiles per page (3x2 or 4x3), filled row by row
///        (reading order) and scrolled vertically below the app bar. L/R flip a page.
class DsRunnerTileGridDisplayMode : public RomBrowserDisplayMode
{
public:
    /// 3 columns x 2 rows of 76x70 tiles, icons scaled 2x, games show their 64x64 tile image
    static const DsRunnerTileGridDisplayMode sInstance3x2;
    /// 4 columns x 3 rows of 55x44 tiles, only the 32x32 icons at their normal size (no tile images)
    static const DsRunnerTileGridDisplayMode sInstance4x3;

    bool IsVertical() const override { return false; }
    // the DsRunner theme shows a full screen artwork on the top screen instead of the small cover
    bool ShowCoverOnTopScreen() const override { return false; }

    SharedPtr<AppBarView> CreateAppBarView(const IRomBrowserViewFactory* romBrowserViewFactory,
        int startButtonCount, int endButtonCount) const override
    {
        return romBrowserViewFactory->CreateAppBarView(0, 0,
            AppBarView::Orientation::Horizontal, startButtonCount, endButtonCount);
    }

    SharedPtr<RecyclerViewBase> CreateRecyclerView(const IRomBrowserViewFactory* romBrowserViewFactory) const override;

    SharedPtr<FileRecyclerAdapter> CreateRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory, VBlankTextureLoader* vblankTextureLoader) const override
    {
        return SharedPtr<DsRunnerTileGridFileRecyclerAdapter>::MakeShared(viewModel->GetRomBrowserController(),
            &viewModel->GetFileInfoManager(), viewModel->GetIoTaskQueue(),
            themeFileIconFactory, romBrowserViewFactory, vblankTextureLoader,
            TileWidth(), TileHeight(), _iconScale, ClipTop, _useTileImages);
    }

private:
    int _columns;
    int _rows;
    int _xPadding;
    int _yPadding;
    int _spacing;
    int _iconScale;
    // load the tile images of /_dsrunner/tiles (else the tiles only show the icon)
    bool _useTileImages;

    constexpr DsRunnerTileGridDisplayMode(int columns, int rows, int xPadding, int yPadding, int spacing,
        int iconScale, bool useTileImages)
        : _columns(columns), _rows(rows), _xPadding(xPadding), _yPadding(yPadding)
        , _spacing(spacing), _iconScale(iconScale), _useTileImages(useTileImages) { }

    // the grid area is the screen below the app bar
    static constexpr int GridY = 40;
    static constexpr int GridHeight = 192 - GridY;
    // tiles scrolling up are cut off at the app bar
    static constexpr int ClipTop = GridY;

    constexpr int TileWidth() const { return (256 - 2 * _xPadding - (_columns - 1) * _spacing) / _columns; }
    constexpr int TileHeight() const { return (GridHeight - 2 * _yPadding - (_rows - 1) * _spacing) / _rows; }
};

#pragma once
#include "romBrowser/DisplayMode/IconGridFileRecyclerAdapter.h"

/// @brief Adapter of the DsRunner tile grids: large tiles instead of Pico's 44 px icon cells.
///        Console folders get a tile in the color of the console.
class DsRunnerTileGridFileRecyclerAdapter : public IconGridFileRecyclerAdapter
{
public:
    DsRunnerTileGridFileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory, int tileWidth, int tileHeight, int iconScale, int clipTop)
        : IconGridFileRecyclerAdapter(romBrowserController, fileInfoManager, taskQueue,
            themeFileIconFactory, romBrowserViewFactory)
        , _tileWidth(tileWidth), _tileHeight(tileHeight), _iconScale(iconScale), _clipTop(clipTop) { }

    void GetViewSize(int& width, int& height) const override
    {
        width = _tileWidth;
        height = _tileHeight;
    }

    SharedPtr<View> CreateView() const override;
    void BindView(SharedPtr<View> view, int index) const override;

    // the tiles are drawn with the 3D engine and need no shared OBJ graphics
    void InitVram(const VramContext& vramContext) override { }

private:
    int _tileWidth;
    int _tileHeight;
    int _iconScale;
    int _clipTop;
};

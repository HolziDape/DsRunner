#include "common.h"
#include "DsRunnerTileGridDisplayMode.h"

// the small tiles of the 4x3 grid show the icon 1:1 (crisp pixel art) instead of a tile image
const DsRunnerTileGridDisplayMode DsRunnerTileGridDisplayMode::sInstance3x2(3, 2, 8, 3, 6, 2, true);
const DsRunnerTileGridDisplayMode DsRunnerTileGridDisplayMode::sInstance4x3(4, 3, 9, 4, 6, 1, false);

SharedPtr<RecyclerViewBase> DsRunnerTileGridDisplayMode::CreateRecyclerView(
    const IRomBrowserViewFactory* romBrowserViewFactory) const
{
    // the recycler fits as many columns as the width allows: exactly _columns with these tile sizes
    auto recyclerView = RecyclerView::CreateShared(0, GridY, 256, GridHeight, RecyclerView::Mode::VerticalGrid);
    recyclerView->SetPadding(_xPadding, _yPadding);
    recyclerView->SetItemSpacing(_spacing, _spacing);
    return recyclerView;
}

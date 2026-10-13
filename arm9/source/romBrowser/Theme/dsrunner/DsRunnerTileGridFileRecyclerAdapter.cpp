#include "common.h"
#include "romBrowser/FileInfoManager.h"
#include "romBrowser/FileType/FileType.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "romBrowser/IRomBrowserController.h"
#include "DsRunnerLibrary.h"

// tile images mirror the sd card: /Games/NDS/Mario.nds -> /_dsrunner/tiles/Games/NDS/Mario.bmp
#define TILE_FOLDER     "/_dsrunner/tiles"
#include "DsRunnerTileItemView.h"
#include "DsRunnerTileGridFileRecyclerAdapter.h"

SharedPtr<View> DsRunnerTileGridFileRecyclerAdapter::CreateView() const
{
    return DsRunnerTileItemView::CreateShared(std::make_unique<RomBrowserItemViewModel>(_romBrowserController),
        _romBrowserController, _textureLoader, _tileWidth, _tileHeight, _iconScale, _clipTop);
}

void DsRunnerTileGridFileRecyclerAdapter::BindView(SharedPtr<View> view, int index) const
{
    auto tileView = static_cast<DsRunnerTileItemView*>(view.GetPointer());
    const auto& fileInfo = _fileInfoManager->GetItem(index);
    tileView->SetItem(&fileInfo);
    const DsRunnerConsoles::Info* console = nullptr;
    if (fileInfo.GetFileType()->GetClassification() == FileTypeClassification::Folder)
    {
        console = DsRunnerConsoles::FindByFolderName(fileInfo.GetFileName());
    }
    if (console)
        tileView->SetTint(console->tile);
    else
        tileView->ClearTint();
    IconGridFileRecyclerAdapter::BindView(view, index);
}

TaskResult<void> DsRunnerTileGridFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const
{
    auto result = IconGridFileRecyclerAdapter::BindView(view, index, internalFileInfo, cancelRequested);
    if (cancelRequested)
        return result;

    // games can have a tile image instead of the icon (folders keep their console badge)
    const auto& fileInfo = _fileInfoManager->GetItem(index);
    if (fileInfo.GetFileType()->GetClassification() != FileTypeClassification::Folder)
    {
        char path[256];
        if (DsRunnerPaths::MirrorPath(TILE_FOLDER, _romBrowserController->GetItemFolderPath(fileInfo),
                fileInfo.GetFileName(), false, path, sizeof(path)))
        {
            static_cast<DsRunnerTileItemView*>(view.GetPointer())->LoadTileImage(path, cancelRequested);
        }
    }
    return result;
}

void DsRunnerTileGridFileRecyclerAdapter::ReleaseView(SharedPtr<View> view, int index) const
{
    static_cast<DsRunnerTileItemView*>(view.GetPointer())->SetItem(nullptr);
    IconGridFileRecyclerAdapter::ReleaseView(view, index);
}

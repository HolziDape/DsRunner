#include "common.h"
#include "romBrowser/FileInfoManager.h"
#include "romBrowser/FileType/FileType.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "themes/dsrunner/DsRunnerConsoleArt.h"
#include "romBrowser/IRomBrowserController.h"
#include "services/settings/AppSettings.h"
#include "DsRunnerLibrary.h"
#include "DsRunnerSearch.h"

// tile images mirror the sd card: /Games/NDS/Mario.nds -> /_dsrunner/tiles/Games/NDS/Mario.bmp
#define TILE_FOLDER     "/_dsrunner/tiles"
#include "DsRunnerTileItemView.h"
#include "DsRunnerTileGridFileRecyclerAdapter.h"

SharedPtr<View> DsRunnerTileGridFileRecyclerAdapter::CreateView() const
{
    // without a texture loader the view allocates no texture vram and never shows a tile image
    return DsRunnerTileItemView::CreateShared(std::make_unique<RomBrowserItemViewModel>(_romBrowserController),
        _romBrowserController, _useTileImages ? _textureLoader : nullptr, _tileWidth, _tileHeight, _iconScale,
        _clipTop);
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
    else if (_romBrowserController->IsCurrentFolderSearch() && gDsRunnerLibrary.appSettings)
    {
        // the search results show every game in the color of its console
        char consoleFolder[64];
        if (DsRunnerPaths::GetConsoleFolderName(gDsRunnerLibrary.appSettings->libraryPath.GetString(),
                _romBrowserController->GetItemFolderPath(fileInfo), consoleFolder, sizeof(consoleFolder)))
        {
            console = DsRunnerConsoles::FindByFolderName(consoleFolder);
        }
    }
    if (console)
        tileView->SetTint(console->tile);
    else
        tileView->ClearTint();
    if (gDsRunnerConsoleArt.IsAvailable() && fileInfo.GetFileType()->GetClassification() == FileTypeClassification::Folder)
    {
        // the label under the console image: the console's short name, else the folder name
        const char* name = fileInfo.GetFileName();
        tileView->SetLabel(DsRunnerFavoritesFolder::IsEntry(name) ? "Favoriten"
            : DsRunnerSearch::IsEntry(name) ? "Suche" : console ? console->label : name);
    }
    IconGridFileRecyclerAdapter::BindView(view, index);
}

TaskResult<void> DsRunnerTileGridFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const
{
    auto result = IconGridFileRecyclerAdapter::BindView(view, index, internalFileInfo, cancelRequested);
    auto tileView = static_cast<DsRunnerTileItemView*>(view.GetPointer());
    const auto& fileInfo = _fileInfoManager->GetItem(index);
    bool isFolder = fileInfo.GetFileType()->GetClassification() == FileTypeClassification::Folder;
    if (cancelRequested)
        return result;
    if (isFolder)
        tileView->RenderLabel();
    if (!_useTileImages)
        return result;

    // games and folders can have a tile image of their own instead of the icon
    char path[256];
    if (DsRunnerPaths::MirrorPath(TILE_FOLDER, _romBrowserController->GetItemFolderPath(fileInfo),
            fileInfo.GetFileName(), isFolder, path, sizeof(path)) &&
        tileView->LoadTileImage(path, nullptr, cancelRequested))
    {
        return result;
    }
    // folders without one show the built-in image of their console
    if (isFolder && !cancelRequested && gDsRunnerConsoleArt.IsAvailable() &&
        gDsRunnerConsoleArt.GetImagePath(fileInfo.GetFileName(), DsRunnerConsoleArt::ImageType::Tile, path, sizeof(path)))
    {
        auto tint = DsRunnerConsoleArt::GetTileColor(fileInfo.GetFileName());
        tileView->LoadTileImage(path, &tint, cancelRequested);
    }
    return result;
}

void DsRunnerTileGridFileRecyclerAdapter::ReleaseView(SharedPtr<View> view, int index) const
{
    static_cast<DsRunnerTileItemView*>(view.GetPointer())->SetItem(nullptr);
    IconGridFileRecyclerAdapter::ReleaseView(view, index);
}

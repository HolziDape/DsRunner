#include "common.h"
#include "romBrowser/FileInfoManager.h"
#include "romBrowser/FileType/FileType.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "DsRunnerTileItemView.h"
#include "DsRunnerTileGridFileRecyclerAdapter.h"

SharedPtr<View> DsRunnerTileGridFileRecyclerAdapter::CreateView() const
{
    return DsRunnerTileItemView::CreateShared(std::make_unique<RomBrowserItemViewModel>(_romBrowserController),
        _romBrowserController, _tileWidth, _tileHeight, _iconScale, _clipTop);
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

void DsRunnerTileGridFileRecyclerAdapter::ReleaseView(SharedPtr<View> view, int index) const
{
    static_cast<DsRunnerTileItemView*>(view.GetPointer())->SetItem(nullptr);
    IconGridFileRecyclerAdapter::ReleaseView(view, index);
}

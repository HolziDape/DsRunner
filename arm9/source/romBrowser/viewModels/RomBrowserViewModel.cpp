#include "common.h"
#include <algorithm>
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "romBrowser/Theme/dsrunner/DsRunnerLibrary.h"
#include "RomBrowserViewModel.h"

RomBrowserViewModel::RomBrowserViewModel(IRomBrowserController* romBrowserController, const char* initialSelectedFileName)
    : _romBrowserController(romBrowserController)
{
    SdFolderFilterSortParams filterSortParams;
    switch (romBrowserController->GetRomBrowserDisplaySettings().sortMode)
    {
        case RomBrowserSortMode::NameAscending:
        default:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Name, SdFolderSortDirection::Ascending, false);
            break;
        }
        case RomBrowserSortMode::NameDescending:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Name, SdFolderSortDirection::Descending, false);
            break;
        }
        case RomBrowserSortMode::LastModified:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::LastModified, SdFolderSortDirection::Descending, false);
            break;
        }
        case RomBrowserSortMode::LastPlayed:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::LastPlayed, SdFolderSortDirection::Ascending, false);
            filterSortParams.playHistory = &gDsRunnerLibrary.playHistory;
            filterSortParams.folderPath = romBrowserController->GetCurrentFolderPath();
            break;
        }
    }
    u64 startTick = gTickCounter.GetValue();
    const auto& sdFolder = romBrowserController->GetSdFolder();
    u32 filteredCount;
    auto sortedFilteredFiles = sdFolder.FilterAndSort(filterSortParams, filteredCount);
    // DsRunner: the favorites tile comes first on the console level
    DsRunnerFavoritesFolder::MoveEntryToFront(sortedFilteredFiles.get(), filteredCount);
    u64 endTick = gTickCounter.GetValue();
    LOG_DEBUG("Filter + sort took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
    _fileInfoManager = std::make_unique<FileInfoManager>(std::move(sortedFilteredFiles),
        filteredCount, _romBrowserController->GetCoverRepository(), _romBrowserController->GetIconRepository(),
        _romBrowserController->GetBannerRepository());
    _selectedItem = _fileInfoManager->GetItemIndex(initialSelectedFileName);
}

void RomBrowserViewModel::NavigateUp()
{
    _romBrowserController->NavigateUp();
}

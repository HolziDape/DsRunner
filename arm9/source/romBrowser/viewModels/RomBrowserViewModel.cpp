#include "common.h"
#include <algorithm>
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "romBrowser/Theme/dsrunner/DsRunnerLibrary.h"
#include "romBrowser/Theme/dsrunner/DsRunnerSearch.h"
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
    // DsRunner: the favorites and the search results have files of several folders: the play history
    // needs the folder of each file, and the search results stay grouped by console
    if (romBrowserController->IsCurrentFolderVirtual())
    {
        filterSortParams.sortContext = romBrowserController;
        filterSortParams.getFolderPath = [] (const FileInfo* file, const void* context)
        {
            return (const char*)((const IRomBrowserController*)context)->GetItemFolderPath(*file);
        };
        filterSortParams.getGroup = [] (const FileInfo* file, const void* context)
        {
            return ((const IRomBrowserController*)context)->GetItemGroup(*file);
        };
    }
    u64 startTick = gTickCounter.GetValue();
    const auto& sdFolder = romBrowserController->GetSdFolder();
    u32 filteredCount;
    auto sortedFilteredFiles = sdFolder.FilterAndSort(filterSortParams, filteredCount);
    // DsRunner: the favorites tile comes first on the console level, then the search tile
    DsRunnerSearch::MoveEntryToFront(sortedFilteredFiles.get(), filteredCount);
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

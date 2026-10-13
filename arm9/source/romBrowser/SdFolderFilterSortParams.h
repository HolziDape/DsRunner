#pragma once
#include "SdFolderSortType.h"
#include "SdFolderSortDirection.h"

class DsRunnerPlayHistory;

class SdFolderFilterSortParams
{
public:
    SdFolderSortType sortType = SdFolderSortType::Name;
    SdFolderSortDirection sortDirection = SdFolderSortDirection::Ascending;
    bool includeHiddenFiles = false;
    // DsRunner: for SdFolderSortType::LastPlayed, the play history and the path of the folder being sorted
    const DsRunnerPlayHistory* playHistory = nullptr;
    const char* folderPath = nullptr;

    SdFolderFilterSortParams() { }

    SdFolderFilterSortParams(SdFolderSortType sortType, SdFolderSortDirection sortDirection, bool includeHiddenFiles)
        : sortType(sortType), sortDirection(sortDirection), includeHiddenFiles(includeHiddenFiles) { }
};

#pragma once
#include "SdFolderSortType.h"
#include "SdFolderSortDirection.h"

class DsRunnerPlayHistory;
class FileInfo;

class SdFolderFilterSortParams
{
public:
    SdFolderSortType sortType = SdFolderSortType::Name;
    SdFolderSortDirection sortDirection = SdFolderSortDirection::Ascending;
    bool includeHiddenFiles = false;
    // DsRunner: for SdFolderSortType::LastPlayed, the play history and the path of the folder being sorted
    const DsRunnerPlayHistory* playHistory = nullptr;
    const char* folderPath = nullptr;
    // DsRunner: virtual folders (favorites, search results) show files of several folders. When set,
    // getFolderPath gives the folder of each file for the play history (instead of folderPath), and
    // getGroup a group the files are sorted by first (the console of a search result), with sortContext.
    const char* (*getFolderPath)(const FileInfo* file, const void* sortContext) = nullptr;
    int (*getGroup)(const FileInfo* file, const void* sortContext) = nullptr;
    const void* sortContext = nullptr;

    SdFolderFilterSortParams() { }

    SdFolderFilterSortParams(SdFolderSortType sortType, SdFolderSortDirection sortDirection, bool includeHiddenFiles)
        : sortType(sortType), sortDirection(sortDirection), includeHiddenFiles(includeHiddenFiles) { }
};

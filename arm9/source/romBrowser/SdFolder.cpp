#include "common.h"
#include <string.h>
#include <algorithm>
#include "Theme/dsrunner/DsRunnerLibrary.h"
#include "SdFolder.h"

SdFolder::SdFolder(FileInfo** files, int fileCount)
    : _files(files), _fileCount(fileCount) { }

SdFolder::~SdFolder()
{
    for (int i = 0; i < _fileCount; i++)
        delete _files[i];
    free(_files);
}

std::unique_ptr<const FileInfo*[]> SdFolder::FilterAndSort(
    const SdFolderFilterSortParams& filterSortParams, u32& resultCount) const
{
    auto sortedFilteredFiles = std::make_unique<const FileInfo*[]>(_fileCount);
    u32 filteredCount = 0;
    for (int i = 0; i < _fileCount; i++)
    {
        const FileInfo* file = _files[i];
        bool isHidden = file->GetFileName()[0] == '.' || file->IsHidden();
        auto classification = file->GetFileType()->GetClassification();
        if (classification != FileTypeClassification::Unknown &&
            (!isHidden || filterSortParams.includeHiddenFiles))
        {
            sortedFilteredFiles[filteredCount++] = file;
        }
    }
    // DsRunner: last played, and the virtual folders (favorites, search results, see getGroup)
    if (filterSortParams.sortType == SdFolderSortType::LastPlayed || filterSortParams.getGroup)
    {
        SortWithKeys(filterSortParams, sortedFilteredFiles.get(), filteredCount);
        resultCount = filteredCount;
        return sortedFilteredFiles;
    }
    std::sort(sortedFilteredFiles.get(), sortedFilteredFiles.get() + filteredCount,
        [filterSortParams] (const FileInfo*& a, const FileInfo*& b)
        {
            bool result = true;
            if (CompareClassification(a, b, result))
                return result;

            auto sortType = filterSortParams.sortType;
            auto sortDirection = filterSortParams.sortDirection;
            if (a->GetFileType()->GetClassification() == FileTypeClassification::Folder &&
                b->GetFileType()->GetClassification() == FileTypeClassification::Folder)
            {
                if (sortType != SdFolderSortType::Name)
                {
                    sortType = SdFolderSortType::Name;
                    sortDirection = SdFolderSortDirection::Ascending;
                }
            }
            switch (sortType)
            {
                case SdFolderSortType::Name:
                default:
                {
                    result = CompareName(a, b);
                    break;
                }
            }
            return sortDirection == SdFolderSortDirection::Ascending ? result : !result;
        });
    resultCount = filteredCount;
    return sortedFilteredFiles;
}

void SdFolder::SortWithKeys(const SdFolderFilterSortParams& filterSortParams,
    const FileInfo** files, u32 count) const
{
    // look up the position in the play history (last played) and the group (virtual folders) once per file,
    // then sort by them: folders first (by name), then by group, then the played files newest first,
    // then the rest by name (by name in the chosen direction when not sorting by last played)
    struct KeyedFile
    {
        int group;
        u32 rank;
        const FileInfo* file;
    };
    bool byLastPlayed = filterSortParams.sortType == SdFolderSortType::LastPlayed;
    bool descending = !byLastPlayed && filterSortParams.sortDirection == SdFolderSortDirection::Descending;
    auto keyedFiles = std::make_unique<KeyedFile[]>(count);
    for (u32 i = 0; i < count; i++)
    {
        int rank = -1;
        bool isFolder = files[i]->GetFileType()->GetClassification() == FileTypeClassification::Folder;
        if (byLastPlayed && filterSortParams.playHistory && !isFolder)
        {
            // DsRunner: the files of virtual folders (favorites, search results) come from several folders
            const char* folderPath = filterSortParams.getFolderPath
                ? filterSortParams.getFolderPath(files[i], filterSortParams.sortContext)
                : filterSortParams.folderPath;
            if (folderPath)
                rank = filterSortParams.playHistory->GetRank(folderPath, files[i]->GetFileName());
        }
        int group = filterSortParams.getGroup ? filterSortParams.getGroup(files[i], filterSortParams.sortContext) : 0;
        keyedFiles[i] = { group, rank < 0 ? 0xFFFFFFFFu : (u32)rank, files[i] };
    }
    std::sort(keyedFiles.get(), keyedFiles.get() + count,
        [descending] (const KeyedFile& a, const KeyedFile& b)
        {
            bool result = true;
            if (CompareClassification(a.file, b.file, result))
                return result;
            if (a.group != b.group)
                return a.group < b.group;
            if (a.rank != b.rank)
                return a.rank < b.rank;
            // folders always by name ascending, like Pico does
            if (descending && a.file->GetFileType()->GetClassification() != FileTypeClassification::Folder)
                return CompareName(b.file, a.file);
            return CompareName(a.file, b.file);
        });
    for (u32 i = 0; i < count; i++)
        files[i] = keyedFiles[i].file;
}

bool SdFolder::CompareClassification(const FileInfo* a, const FileInfo* b, bool& result)
{
    auto aClassification = a->GetFileType()->GetClassification();
    auto bClassification = b->GetFileType()->GetClassification();
    if (aClassification == bClassification)
        return false;

    if (aClassification == FileTypeClassification::Folder)
    {
        result = true;
        return true;
    }
    else if (bClassification == FileTypeClassification::Folder)
    {
        result = false;
        return true;
    }

    return false;
}

bool SdFolder::CompareName(const FileInfo* a, const FileInfo* b)
{
    return strcasecmp(a->GetFileName(), b->GetFileName()) < 0;
}

void SdFolder::AddFile(FileInfo* file)
{
    // the array comes from malloc (see SdFolderFactory)
    _files = (FileInfo**)realloc(_files, sizeof(FileInfo*) * (_fileCount + 1));
    _files[_fileCount++] = file;
}

void SdFolder::SortByNameInPlace()
{
    std::sort(_files, _files + _fileCount, CompareName);
}

const FileInfo* SdFolder::BinarySearch(const char* fileName) const
{
    if (_fileCount != 0)
    {
        const auto file = std::lower_bound(_files, _files + _fileCount, fileName,
            [] (const FileInfo* entry, const char* value)
            {
                return strcasecmp(entry->GetFileName(), value) < 0;
            });

        if (file != _files + _fileCount && !strcasecmp((*file)->GetFileName(), fileName))
        {
            return *file;
        }
    }

    return nullptr;
}

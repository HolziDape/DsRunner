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
    if (filterSortParams.sortType == SdFolderSortType::LastPlayed)
    {
        SortByLastPlayed(filterSortParams, sortedFilteredFiles.get(), filteredCount);
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

void SdFolder::SortByLastPlayed(const SdFolderFilterSortParams& filterSortParams,
    const FileInfo** files, u32 count) const
{
    // look up the position in the play history once per file, then sort by it:
    // folders first (by name), then the played files newest first, then the rest by name
    struct RankedFile
    {
        u32 rank;
        const FileInfo* file;
    };
    auto rankedFiles = std::make_unique<RankedFile[]>(count);
    for (u32 i = 0; i < count; i++)
    {
        int rank = -1;
        if (filterSortParams.playHistory && filterSortParams.folderPath &&
            files[i]->GetFileType()->GetClassification() != FileTypeClassification::Folder)
        {
            rank = filterSortParams.playHistory->GetRank(filterSortParams.folderPath, files[i]->GetFileName());
        }
        rankedFiles[i] = { rank < 0 ? 0xFFFFFFFFu : (u32)rank, files[i] };
    }
    std::sort(rankedFiles.get(), rankedFiles.get() + count,
        [] (const RankedFile& a, const RankedFile& b)
        {
            bool result = true;
            if (CompareClassification(a.file, b.file, result))
                return result;
            if (a.rank != b.rank)
                return a.rank < b.rank;
            return CompareName(a.file, b.file);
        });
    for (u32 i = 0; i < count; i++)
        files[i] = rankedFiles[i].file;
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

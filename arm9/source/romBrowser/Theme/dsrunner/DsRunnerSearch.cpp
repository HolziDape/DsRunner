#include "common.h"
#include <string.h>
#include <algorithm>
#include "fat/Directory.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/FileType/FileType.h"
#include "romBrowser/FileType/IFileTypeProvider.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "DsRunnerLibrary.h"
#include "DsRunnerSearch.h"

// first sizes of the growing arrays
#define INITIAL_GAME_CAPACITY   128
#define INITIAL_NAMES_CAPACITY  4096

DsRunnerSearch gDsRunnerSearch;

// U+00C0 to U+00FF (UTF-8 C3 80 to C3 BF) without accents: '-' is dropped, 'S' is "ss", 'A' is "ae"
static constexpr char sLatin1Letters[] =
    "aaaaaaAceeeeiiiidnooooo-ouuuuy-S"
    "aaaaaaAceeeeiiiidnooooo-ouuuuy-y";

bool DsRunnerSearch::IsEntry(const char* name)
{
    return name && strcmp(name, EntryName) == 0;
}

void DsRunnerSearch::AddEntry(SdFolder& libraryRoot)
{
    DsRunnerVirtualEntries::Add(libraryRoot, EntryName);
}

void DsRunnerSearch::MoveEntryToFront(const FileInfo** files, u32 count)
{
    DsRunnerVirtualEntries::MoveToFront(files, count, EntryName);
}

void DsRunnerSearch::Normalize(const char* text, u32 length, char* result)
{
    u32 resultLength = 0;
    for (u32 i = 0; i < length && resultLength + 2 < NormalizedSize; i++)
    {
        u8 c = text[i];
        if (c < 0x80)
        {
            if (c >= 'A' && c <= 'Z')
                result[resultLength++] = c - 'A' + 'a';
            else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
                result[resultLength++] = c;
            continue;
        }
        if (c == 0xC3 && i + 1 < length && ((u8)text[i + 1] & 0xC0) == 0x80)
        {
            char letter = sLatin1Letters[(u8)text[++i] - 0x80];
            if (letter == 'S')
            {
                result[resultLength++] = 's';
                result[resultLength++] = 's';
            }
            else if (letter == 'A')
            {
                result[resultLength++] = 'a';
                result[resultLength++] = 'e';
            }
            else if (letter != '-')
            {
                result[resultLength++] = letter;
            }
            continue;
        }
        // other characters (and the continuation bytes of their UTF-8 sequence) don't count
    }
    result[resultLength] = 0;
}

void DsRunnerSearch::Clear()
{
    _ready = false;
    free(_games);
    _games = nullptr;
    _gameCount = 0;
    _gameCapacity = 0;
    free(_names);
    _names = nullptr;
    _namesLength = 0;
    _namesCapacity = 0;
    for (u32 i = 0; i < _folderCount; i++)
        _folders[i].path.reset();
    _folderCount = 0;
    _groupCount = 0;
}

bool DsRunnerSearch::AddFolder(const char* parentPath, const char* name, u32 group, u32 depth)
{
    if (_folderCount >= MaxFolders)
        return false;
    u32 parentLength = strlen(parentPath);
    while (parentLength > 0 && parentPath[parentLength - 1] == '/')
        parentLength--;
    u32 nameLength = name ? strlen(name) : 0;
    auto path = std::make_unique<char[]>(parentLength + 1 + nameLength + 1);
    memcpy(path.get(), parentPath, parentLength);
    u32 length = parentLength;
    if (nameLength > 0)
    {
        path[length++] = '/';
        memcpy(&path[length], name, nameLength);
        length += nameLength;
    }
    path[length] = 0;
    if (length == 0)
        strcpy(path.get(), "/");
    auto& folder = _folders[_folderCount++];
    folder.path = std::move(path);
    folder.group = group;
    folder.depth = depth;
    return true;
}

bool DsRunnerSearch::AddGame(const char* name, const FileType* fileType, const FastFileRef& fastFileRef,
    u8 attributes, u32 folder)
{
    if (_gameCount >= MaxGames)
        return false;
    u32 nameSize = strlen(name) + 1;
    if (_namesLength + nameSize > MaxNameBytes)
        return false;
    if (_gameCount == _gameCapacity)
    {
        u32 capacity = std::min<u32>(_gameCapacity ? _gameCapacity * 2 : INITIAL_GAME_CAPACITY, MaxGames);
        auto games = (Game*)realloc(_games, capacity * sizeof(Game));
        if (!games)
            return false;
        _games = games;
        _gameCapacity = capacity;
    }
    if (_namesLength + nameSize > _namesCapacity)
    {
        u32 capacity = _namesCapacity ? _namesCapacity : INITIAL_NAMES_CAPACITY;
        while (capacity < _namesLength + nameSize)
            capacity *= 2;
        capacity = std::min<u32>(capacity, MaxNameBytes);
        auto names = (char*)realloc(_names, capacity);
        if (!names)
            return false;
        _names = names;
        _namesCapacity = capacity;
    }
    memcpy(&_names[_namesLength], name, nameSize);
    auto& game = _games[_gameCount++];
    game.fastFileRef = fastFileRef;
    game.fileType = fileType;
    game.nameOffset = _namesLength;
    game.folder = folder;
    game.attributes = attributes;
    _namesLength += nameSize;
    return true;
}

void DsRunnerSearch::ReadFolder(u32 folder, const IFileTypeProvider& fileTypeProvider, const vu8& cancelRequested)
{
    // FatFs with long file names needs a lot of stack already: nothing big on the io thread stack
    auto directory = std::make_unique<Directory>();
    auto fileInfo = std::make_unique<FILINFO>();
    if (directory->Open(_folders[folder].path.get()) != FR_OK)
        return;
    bool isLibraryRoot = folder == 0;
    while (!cancelRequested && directory->Read(fileInfo.get()) == FR_OK && fileInfo->fname[0] != 0)
    {
        // the same files the rom browser shows: nothing hidden, known types
        if ((fileInfo->fattrib & AM_HID) || fileInfo->fname[0] == '.')
            continue;
        if (fileInfo->fattrib & AM_DIR)
        {
            if (isLibraryRoot)
            {
                // a console folder: a group of its own (the last group stays free for the library root)
                if (_groupCount + 1 < MaxGroups && AddFolder(_folders[0].path.get(), fileInfo->fname, _groupCount, 0))
                {
                    auto& group = _groups[_groupCount++];
                    strlcpy(group.folderName, fileInfo->fname, sizeof(group.folderName));
                    const auto* console = DsRunnerConsoles::FindByFolderName(fileInfo->fname);
                    strlcpy(group.label, console ? console->label : fileInfo->fname, sizeof(group.label));
                    // don't end the label in the middle of a UTF-8 character
                    u32 length = strlen(group.label);
                    if (length == sizeof(group.label) - 1)
                    {
                        while (length > 0 && ((u8)group.label[length] & 0xC0) == 0x80)
                            length--;
                        group.label[length] = 0;
                    }
                }
            }
            else if (_folders[folder].depth < MaxDepth)
            {
                AddFolder(_folders[folder].path.get(), fileInfo->fname, _folders[folder].group, _folders[folder].depth + 1);
            }
            continue;
        }
        auto fileType = fileTypeProvider.GetFileType(fileInfo->fname);
        if (!fileType || fileType->GetClassification() == FileTypeClassification::Unknown)
            continue;
        if (!AddGame(fileInfo->fname, fileType, FastFileRef(directory->GetFatFsDirectory(), fileInfo.get()),
                fileInfo->fattrib, folder))
        {
            LOG_DEBUG("Search: index full\n");
            return;
        }
    }
}

void DsRunnerSearch::SortGroups()
{
    // console folders by name like the console level, the games of the library root last
    u8 order[MaxGroups];
    for (u32 i = 0; i < _groupCount; i++)
        order[i] = i;
    u32 rootGroup = _groupCount - 1;
    std::sort(order, order + _groupCount, [this, rootGroup] (u8 a, u8 b)
    {
        if (a == rootGroup || b == rootGroup)
            return b == rootGroup && a != rootGroup;
        return strcasecmp(_groups[a].folderName, _groups[b].folderName) < 0;
    });
    u8 rank[MaxGroups];
    Group sorted[MaxGroups];
    for (u32 i = 0; i < _groupCount; i++)
    {
        rank[order[i]] = i;
        sorted[i] = _groups[order[i]];
    }
    for (u32 i = 0; i < _groupCount; i++)
        _groups[i] = sorted[i];
    for (u32 i = 0; i < _folderCount; i++)
        _folders[i].group = rank[_folders[i].group];
}

bool DsRunnerSearch::Build(const char* libraryPath, const IFileTypeProvider& fileTypeProvider,
    const vu8& cancelRequested)
{
    libraryPath = DsRunnerPaths::SkipDrive(libraryPath);
    if (_ready && DsRunnerPaths::IsSameFolder(libraryPath, _libraryPath))
        return true;
    Clear();
    strlcpy(_libraryPath, libraryPath, sizeof(_libraryPath));
    u64 startTick = gTickCounter.GetValue();

    // folder 0 is the library itself: its subfolders become the groups, its games the last group
    AddFolder(_libraryPath, nullptr, 0, 0);
    ReadFolder(0, fileTypeProvider, cancelRequested);
    auto& rootGroup = _groups[_groupCount++];
    rootGroup.folderName[0] = 0;
    strcpy(rootGroup.label, "Weitere");
    _folders[0].group = _groupCount - 1;
    SortGroups();

    // the folder list grows while it is read: subfolders are added at its end
    for (u32 i = 1; i < _folderCount && !cancelRequested; i++)
        ReadFolder(i, fileTypeProvider, cancelRequested);
    if (cancelRequested)
        return false;

    std::sort(_games, _games + _gameCount, [this] (const Game& a, const Game& b)
    {
        u32 groupA = _folders[a.folder].group;
        u32 groupB = _folders[b.folder].group;
        if (groupA != groupB)
            return groupA < groupB;
        return strcasecmp(&_names[a.nameOffset], &_names[b.nameOffset]) < 0;
    });
    u64 endTick = gTickCounter.GetValue();
    LOG_DEBUG("Search: %d games in %d folders, %d bytes of names, %d us\n", _gameCount, _folderCount,
        _namesLength, (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
    _ready = true;
    return true;
}

bool DsRunnerSearch::Matches(const Game& game, const char* normalizedQuery) const
{
    const char* name = &_names[game.nameOffset];
    // the name without extension
    const char* extension = strrchr(name, '.');
    u32 length = extension && extension != name ? extension - name : strlen(name);
    char normalized[NormalizedSize];
    Normalize(name, length, normalized);
    return strstr(normalized, normalizedQuery) != nullptr;
}

u32 DsRunnerSearch::Count(const char* normalizedQuery, u16* groupCounts) const
{
    if (groupCounts)
        memset(groupCounts, 0, sizeof(u16) * MaxGroups);
    if (!_ready || normalizedQuery[0] == 0)
        return 0;
    u32 count = 0;
    for (u32 i = 0; i < _gameCount; i++)
    {
        if (Matches(_games[i], normalizedQuery))
        {
            count++;
            if (groupCounts)
                groupCounts[_folders[_games[i].folder].group]++;
        }
    }
    return count;
}

std::unique_ptr<DsRunnerSearchResults> DsRunnerSearch::CreateResults(const char* normalizedQuery, int group,
    std::unique_ptr<SdFolder>& folder) const
{
    auto results = std::make_unique<DsRunnerSearchResults>();
    results->_folders = std::make_unique<std::unique_ptr<char[]>[]>(std::max<u32>(_folderCount, 1));
    results->_items = std::make_unique<DsRunnerSearchResults::Item[]>(MaxResults);
    // the sd folder takes the array over and frees it with free()
    auto files = (FileInfo**)malloc(sizeof(FileInfo*) * MaxResults);
    u32 fileCount = 0;
    if (_ready && normalizedQuery[0] != 0)
    {
        for (u32 i = 0; i < _gameCount; i++)
        {
            const auto& game = _games[i];
            if (!Matches(game, normalizedQuery))
                continue;
            u32 gameGroup = _folders[game.folder].group;
            results->_totalCount++;
            results->_groupCounts[gameGroup]++;
            if (group >= 0 && gameGroup != (u32)group)
                continue;
            results->_filterCount++;
            if (fileCount >= MaxResults)
                continue;
            auto file = new FileInfo(&_names[game.nameOffset], game.fileType, game.fastFileRef, game.attributes);
            files[fileCount] = file;
            results->_items[fileCount] = { file, game.folder, (u8)gameGroup };
            fileCount++;
            // the results keep their own copy of the folder paths
            auto& folderPath = results->_folders[game.folder];
            if (!folderPath)
            {
                const char* path = _folders[game.folder].path.get();
                folderPath = std::make_unique<char[]>(strlen(path) + 1);
                strcpy(folderPath.get(), path);
            }
        }
    }
    results->_itemCount = fileCount;
    LOG_DEBUG("Search: %d of %d matches shown\n", fileCount, results->_totalCount);
    folder = std::make_unique<SdFolder>(files, fileCount);
    return results;
}

const char* DsRunnerSearchResults::GetFolderPath(const FileInfo* file) const
{
    for (u32 i = 0; i < _itemCount; i++)
    {
        if (_items[i].file == file)
            return _folders[_items[i].folder].get();
    }
    return nullptr;
}

int DsRunnerSearchResults::GetGroup(const FileInfo* file) const
{
    for (u32 i = 0; i < _itemCount; i++)
    {
        if (_items[i].file == file)
            return _items[i].group;
    }
    return -1;
}

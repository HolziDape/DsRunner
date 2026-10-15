#pragma once
#include <memory>
#include "common.h"
#include "fat/FastFileRef.h"

class SdFolder;
class FileInfo;
class FileType;
class IFileTypeProvider;
class DsRunnerSearchResults;

/// @brief Search over the games of all console folders ("Suche" on the console level).
///        The first search reads the library once on the io thread into a compact index: name, file type
///        and FastFileRef of every game (about 32 bytes plus the name per game, at most MaxGames games and
///        MaxNameBytes of names). It stays until the launcher exits, the files don't change while DsRunner
///        runs. Counting while typing then needs no sd access, and the result list is built from the index.
///        Indexed are the same files a console folder shows (known file type, not hidden), in the console
///        folders and up to MaxDepth levels of subfolders below them, plus the games directly in the library.
///        Matching compares the file name without extension, ignoring case, accents (ä = a, ß = ss) and all
///        characters that are no letter or digit ("mariokart" finds "Mario_Kart DS.nds").
class DsRunnerSearch
{
public:
    static constexpr u32 MaxGames = 2000;
    static constexpr u32 MaxNameBytes = 64 * 1024;
    static constexpr u32 MaxFolders = 96;
    /// Console folders (groups), the games directly in the library are one more group, sorted last.
    static constexpr u32 MaxGroups = 32;
    /// Subfolder levels below a console folder that are searched.
    static constexpr u32 MaxDepth = 2;
    /// More results than this are not shown (the counts stay exact).
    static constexpr u32 MaxResults = 200;
    static constexpr u32 MaxQueryLength = 24;
    /// Size of a normalized query or name buffer.
    static constexpr u32 NormalizedSize = 256;

    /// Name of the entry on the console level (UTF-8, "• Suche"). Thanks to the bullet no real folder
    /// is called like this.
    static constexpr const char* EntryName = "\xE2\x80\xA2 Suche";

    /// @brief Returns true when \p name is the name of the search entry.
    static bool IsEntry(const char* name);

    /// @brief Adds the search entry to the files of the library root. Io thread.
    static void AddEntry(SdFolder& libraryRoot);

    /// @brief Moves the search entry to the front of a sorted file list.
    static void MoveEntryToFront(const FileInfo** files, u32 count);

    /// @brief Writes the comparable form of \p text (UTF-8, \p length bytes) to \p result: lower case
    ///        letters and digits only, accents removed, ß = ss. \p result has NormalizedSize bytes.
    static void Normalize(const char* text, u32 length, char* result);

    /// @brief Reads the library into the index unless that was done already. Io thread.
    /// @return True when the index is ready.
    bool Build(const char* libraryPath, const IFileTypeProvider& fileTypeProvider, const vu8& cancelRequested);

    /// @brief Returns true when the index is complete. Only then the main thread may use it.
    bool IsReady() const { return _ready; }

    /// @brief Gets the number of indexed games (index ready).
    u32 GetGameCount() const { return _gameCount; }

    /// @brief Gets the number of console groups (index ready).
    u32 GetGroupCount() const { return _groupCount; }

    /// @brief Gets the short name of a group, e.g. "GBA" (index ready).
    const char* GetGroupLabel(u32 group) const { return group < _groupCount ? _groups[group].label : ""; }

    /// @brief Counts the games that match a normalized query (index ready).
    /// @param normalizedQuery The query, see Normalize(). An empty query matches nothing.
    /// @param groupCounts Receives the number of matches per group (MaxGroups entries), may be null.
    /// @return The number of matches.
    u32 Count(const char* normalizedQuery, u16* groupCounts) const;

    /// @brief Creates the result list (index ready, io thread): the matching games of one group or of
    ///        all groups, sorted by group, then by name, at most MaxResults.
    /// @param normalizedQuery The query, see Normalize().
    /// @param group The group to show, or -1 for all.
    /// @param folder Receives the files.
    std::unique_ptr<DsRunnerSearchResults> CreateResults(const char* normalizedQuery, int group,
        std::unique_ptr<SdFolder>& folder) const;

private:
    struct Game
    {
        FastFileRef fastFileRef;
        const FileType* fileType;
        u32 nameOffset;
        u8 folder;
        u8 attributes;
    };

    struct Folder
    {
        std::unique_ptr<char[]> path;
        u8 group;
        u8 depth;
    };

    struct Group
    {
        char folderName[64];
        char label[16];
    };

    volatile bool _ready = false;
    char _libraryPath[256] = { 0 };
    Game* _games = nullptr;
    u32 _gameCount = 0;
    u32 _gameCapacity = 0;
    char* _names = nullptr;
    u32 _namesLength = 0;
    u32 _namesCapacity = 0;
    Folder _folders[MaxFolders];
    u32 _folderCount = 0;
    Group _groups[MaxGroups];
    u32 _groupCount = 0;

    void Clear();
    bool AddFolder(const char* parentPath, const char* name, u32 group, u32 depth);
    bool AddGame(const char* name, const FileType* fileType, const FastFileRef& fastFileRef, u8 attributes, u32 folder);
    void ReadFolder(u32 folder, const IFileTypeProvider& fileTypeProvider, const vu8& cancelRequested);
    void SortGroups();
    bool Matches(const Game& game, const char* normalizedQuery) const;
};

/// @brief The content of the search folder: which folder each result is in, and the counts of the query.
class DsRunnerSearchResults
{
public:
    /// @brief Gets the folder that contains \p file (one of the result files), or nullptr.
    const char* GetFolderPath(const FileInfo* file) const;

    /// @brief Gets the group (console) of a result file, or -1.
    int GetGroup(const FileInfo* file) const;

    /// @brief The number of games that match the query, in all groups.
    u32 GetTotalCount() const { return _totalCount; }

    /// @brief The number of games of a group that match the query.
    u32 GetGroupMatchCount(u32 group) const { return group < DsRunnerSearch::MaxGroups ? _groupCounts[group] : 0; }

    /// @brief The number of matches in the shown group (or all groups).
    u32 GetFilterMatchCount() const { return _filterCount; }

    /// @brief The number of files in the list (at most DsRunnerSearch::MaxResults).
    u32 GetShownCount() const { return _itemCount; }

private:
    friend class DsRunnerSearch;

    struct Item
    {
        const FileInfo* file;
        u8 folder;
        u8 group;
    };

    std::unique_ptr<std::unique_ptr<char[]>[]> _folders;
    std::unique_ptr<Item[]> _items;
    u32 _itemCount = 0;
    u32 _totalCount = 0;
    u32 _filterCount = 0;
    u16 _groupCounts[DsRunnerSearch::MaxGroups] = { 0 };
};

extern DsRunnerSearch gDsRunnerSearch;

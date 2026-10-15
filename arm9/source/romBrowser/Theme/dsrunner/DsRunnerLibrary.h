#pragma once
#include <memory>
#include "common.h"

class SdFolder;
class FileInfo;
class IFileTypeProvider;
class TaskQueueBase;
class AppSettings;

namespace DsRunnerPaths
{
    /// @brief Skips a drive prefix like "fat:" of a path.
    const char* SkipDrive(const char* path);

    /// @brief Returns true when \p a and \p b are the same folder, ignoring case,
    ///        drive prefix and a trailing slash.
    bool IsSameFolder(const char* a, const char* b);

    /// @brief Builds "<folderPath>/<fileName>" without the drive prefix of \p folderPath.
    /// @return False when the path doesn't fit in \p result.
    bool Combine(const char* folderPath, const char* fileName, char* result, u32 resultSize);

    /// @brief Builds the path of a DsRunner image that mirrors the sd card under \p root, e.g. root
    ///        "/_dsrunner/art", folder "fat:/Games/NDS", file "Mario.nds" -> "/_dsrunner/art/Games/NDS/Mario.bmp".
    ///        Files lose their extension, folder names are kept as they are.
    /// @return False when the path doesn't fit in \p result.
    bool MirrorPath(const char* root, const char* folderPath, const char* name, bool isFolder,
        char* result, u32 resultSize);

    /// @brief Gets the console folder of a folder in the library: the first folder name of \p folderPath
    ///        below \p libraryPath, e.g. "GBA" for "/Games/GBA/Hacks" in "/Games".
    /// @return False when \p folderPath is not below \p libraryPath or the name doesn't fit.
    bool GetConsoleFolderName(const char* libraryPath, const char* folderPath, char* result, u32 resultSize);
}

/// @brief Entries of the console level that are no real folders ("★ Favoriten", "• Suche").
namespace DsRunnerVirtualEntries
{
    /// @brief Adds a folder entry called \p name to the files of the library root. Io thread.
    void Add(SdFolder& libraryRoot, const char* name);

    /// @brief Moves the entry called \p name to the front of a sorted file list.
    void MoveToFront(const FileInfo** files, u32 count, const char* name);
}

/// @brief Play history: the most recently started files, newest first.
///        Stored as one path per line in /_dsrunner/recent.txt. Used for sorting by "last played".
class DsRunnerPlayHistory
{
public:
    static constexpr u32 MaxEntries = 100;

    /// @brief Reads the history file once. Call from the io thread.
    void Load();

    /// @brief Moves \p path to the front of the history and writes the history file. Call from the io thread.
    void Add(const char* path);

    /// @brief Gets the position of a file in the history.
    /// @param folderPath The folder of the file.
    /// @param fileName The name of the file.
    /// @return 0 for the last played file, 1 for the one before and so on, or -1 when it was never played.
    int GetRank(const char* folderPath, const char* fileName) const;

    u32 GetCount() const { return _count; }

    /// @brief Gets an entry (full path without drive prefix), 0 is the last played file.
    const char* Get(u32 index) const { return index < _count ? _entries[index].get() : nullptr; }

private:
    bool _loaded = false;
    u32 _count = 0;
    std::unique_ptr<char[]> _entries[MaxEntries];

    void Save() const;
};

/// @brief A date and time of the DS clock (local time, as set in the DS settings).
struct DsRunnerDateTime
{
    u16 year = 0;
    u8 month = 0;
    u8 day = 0;
    u8 hour = 0;
    u8 minute = 0;

    /// @brief Reads the DS clock (an ipc request to the arm7 that blocks the caller for a moment).
    static DsRunnerDateTime Now();

    /// @brief False for an unknown date (year 0).
    bool IsValid() const { return year != 0 && month >= 1 && month <= 12 && day >= 1 && day <= 31; }

    /// @brief Gets the number of days since 1970-01-01, for the days between two dates.
    int GetDayNumber() const;
};

/// @brief Play statistics: how often each file was started and when the last time.
///        Stored in /_dsrunner/stats.txt, one file per line, the last played first:
///        "<path>|<count>|<YYYY-MM-DD HH:MM>" ("|" can't be part of a file name on FAT).
///        Files that were never started are not in the list, at most MaxEntries files are kept.
class DsRunnerPlayStats
{
public:
    static constexpr u32 MaxEntries = 500;
    static constexpr u32 MaxCount = 9999;

    struct Entry
    {
        /// Full path without drive prefix.
        std::unique_ptr<char[]> path;
        u16 count = 0;
        DsRunnerDateTime lastPlayed;
    };

    /// @brief Reads the statistics file once. Call from the io thread.
    void Load();

    /// @brief Counts a start of \p path at the current time and writes the statistics file. Call from the io thread.
    void Add(const char* path);

    /// @brief Gets the statistics of the file \p fileName in \p folderPath, or nullptr when it was never started
    ///        (or was started before DsRunner kept statistics).
    const Entry* Find(const char* folderPath, const char* fileName) const;

    u32 GetCount() const { return _count; }

private:
    bool _loaded = false;
    u32 _count = 0;
    Entry _entries[MaxEntries];

    void Save() const;
};

/// @brief Favorite games, toggled with X. Stored as one path per line in /_dsrunner/favorites.txt
///        in the order they were added.
class DsRunnerFavorites
{
public:
    static constexpr u32 MaxEntries = 200;

    /// @brief Reads the favorites file once. Call from the io thread before the first folder is shown.
    void Load();

    /// @brief Adds or removes a file. Call from the main thread, the file is written on \p ioTaskQueue.
    /// @return True when the file is a favorite now.
    bool Toggle(const char* folderPath, const char* fileName, TaskQueueBase* ioTaskQueue);

    /// @brief Returns true when the file \p fileName in \p folderPath is a favorite.
    bool Contains(const char* folderPath, const char* fileName) const
    {
        return Find(folderPath, fileName) >= 0;
    }

    u32 GetCount() const { return _count; }

    /// @brief Gets a favorite (full path without drive prefix).
    const char* Get(u32 index) const { return index < _count ? _entries[index].get() : nullptr; }

    /// @brief Changes whenever a favorite is added or removed, for views that cache the favorite state.
    u32 GetVersion() const { return _version; }

private:
    bool _loaded = false;
    u32 _count = 0;
    u32 _version = 0;
    std::unique_ptr<char[]> _entries[MaxEntries];

    int Find(const char* folderPath, const char* fileName) const;
};

/// @brief The virtual folder "★ Favoriten" on the console level. Its entry is added to the
///        library root when there are favorites, opening it lists the favorite files of all folders.
class DsRunnerFavoritesFolder
{
public:
    /// Name of the entry on the console level (UTF-8, "★ Favoriten"). Thanks to the star
    /// no real folder is called like this.
    static constexpr const char* EntryName = "\xE2\x98\x85 Favoriten";

    /// @brief Returns true when \p name is the name of the favorites entry.
    static bool IsEntry(const char* name);

    /// @brief Adds the favorites entry to the files of the library root when there are favorites. Io thread.
    static void AddEntry(SdFolder& libraryRoot, const DsRunnerFavorites& favorites);

    /// @brief Moves the favorites entry to the front of a sorted file list.
    static void MoveEntryToFront(const FileInfo** files, u32 count);

    /// @brief Creates the content of the favorites folder: the favorite files that still exist. Io thread.
    /// @param favorites The favorites.
    /// @param fileTypeProvider Gives the file types, like for a normal folder.
    /// @param folder Receives the files.
    /// @return The folder of each file, for GetFolderPath().
    static std::unique_ptr<DsRunnerFavoritesFolder> Create(const DsRunnerFavorites& favorites,
        const IFileTypeProvider& fileTypeProvider, std::unique_ptr<SdFolder>& folder);

    /// @brief Gets the folder that contains \p file (one of the files made by Create()), or nullptr.
    const char* GetFolderPath(const FileInfo* file) const;

private:
    struct Item
    {
        const FileInfo* file;
        u32 folderIndex;
    };

    std::unique_ptr<std::unique_ptr<char[]>[]> _folders;
    std::unique_ptr<Item[]> _items;
    u32 _itemCount = 0;
};

/// @brief Number of games in each console folder of the library, counted in the background
///        once per library folder and kept until the launcher exits.
class DsRunnerGameCounts
{
public:
    static constexpr u32 MaxFolders = 32;
    static constexpr int Unknown = -1;

    /// @brief Returns true when the subfolders of \p libraryPath still have to be counted.
    bool NeedsCounting(const char* libraryPath) const;

    /// @brief Takes over the subfolders of the library root (main thread, while no counting runs).
    void SetFolders(const char* libraryPath, const SdFolder& folder);

    /// @brief Counts the games in all folders given to SetFolders. Call from the io thread.
    void Count(const IFileTypeProvider& fileTypeProvider, const vu8& cancelRequested);

    /// @brief Gets the number of games of a console folder.
    /// @param folderPath The folder that contains the console folder (must be the library root).
    /// @param folderName The name of the console folder.
    /// @return The number of games, or Unknown when not (yet) counted.
    int Get(const char* folderPath, const char* folderName) const;

    /// @brief Gets the number of games of all console folders of \p libraryPath, or Unknown when not all are counted.
    int GetTotal(const char* libraryPath) const;

private:
    struct Entry
    {
        char name[64];
        volatile int count;
    };

    char _libraryPath[256] = { 0 };
    Entry _entries[MaxFolders];
    u32 _entryCount = 0;
};

/// @brief DsRunner's own data about the library, shared by the rom browser and the theme.
struct DsRunnerLibrary
{
    DsRunnerPlayHistory playHistory;
    DsRunnerPlayStats playStats;
    DsRunnerGameCounts gameCounts;
    DsRunnerFavorites favorites;
    /// The game START continues on the console level ("Weiterspielen"), empty when there is none.
    /// Written by the rom browser controller on the main thread.
    char continuePath[256] = { 0 };
    /// The app settings, for views that show DsRunner options (set by the rom browser controller).
    const AppSettings* appSettings = nullptr;
    /// True while the results of the search are shown (DsRunnerSearch), set by the rom browser controller.
    bool searchResultsShown = false;
};

extern DsRunnerLibrary gDsRunnerLibrary;

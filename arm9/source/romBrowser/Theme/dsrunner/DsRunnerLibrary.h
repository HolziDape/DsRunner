#pragma once
#include <memory>
#include "common.h"

class SdFolder;
class IFileTypeProvider;

namespace DsRunnerPaths
{
    /// @brief Skips a drive prefix like "fat:" of a path.
    const char* SkipDrive(const char* path);

    /// @brief Returns true when \p a and \p b are the same folder, ignoring case,
    ///        drive prefix and a trailing slash.
    bool IsSameFolder(const char* a, const char* b);
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

private:
    bool _loaded = false;
    u32 _count = 0;
    std::unique_ptr<char[]> _entries[MaxEntries];

    void Save() const;
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
    DsRunnerGameCounts gameCounts;
};

extern DsRunnerLibrary gDsRunnerLibrary;

#include "common.h"
#include <string.h>
#include <memory>
#include "fat/Directory.h"
#include "DsRunnerEmulators.h"

using namespace DsRunnerEmulators;

static_assert(kSearchFolderCount <= 8, "the found folders are kept as bits of a u8");

/// Smaller files with an emulator's name are not taken (empty copies, placeholders).
static constexpr u32 kMinEmulatorSize = 16 * 1024;

void DsRunnerEmulatorFinder::Scan()
{
    // found[emulator][file name]: bit n set when the file is in kSearchFolders[n]
    u8 found[kEmulatorCount][3] = { };

    // a missing file costs a full scan of its folder anyway, so read every folder once
    // instead of asking for each candidate path. FILINFO is large, keep it off the 8 KB io stack.
    auto fileInfo = std::make_unique<FILINFO>();
    for (u32 folder = 0; folder < kSearchFolderCount; folder++)
    {
        Directory directory;
        if (directory.Open(kSearchFolders[folder]) != FR_OK)
            continue;
        while (directory.Read(fileInfo.get()) == FR_OK && fileInfo->fname[0] != 0)
        {
            if (fileInfo->fattrib & AM_DIR)
                continue;
            // empty or broken copies (Pico Loader stops with "Failed to load rom header"
            // for files shorter than a rom header); a real emulator has hundreds of KB
            if (fileInfo->fsize < kMinEmulatorSize)
            {
                for (u32 e = 0; e < kEmulatorCount; e++)
                    for (u32 n = 0; n < 3 && kEmulators[e].fileNames[n]; n++)
                        if (!strcasecmp(fileInfo->fname, kEmulators[e].fileNames[n]))
                            LOG_WARNING("DsRunner: %s/%s ignored, only %u bytes\n",
                                kSearchFolders[folder], fileInfo->fname, (unsigned)fileInfo->fsize);
                continue;
            }
            for (u32 e = 0; e < kEmulatorCount; e++)
            {
                for (u32 n = 0; n < 3 && kEmulators[e].fileNames[n]; n++)
                {
                    if (!strcasecmp(fileInfo->fname, kEmulators[e].fileNames[n]))
                        found[e][n] |= 1 << folder;
                }
            }
        }
    }

    // per emulator: the first folder that has one of its files, in the order of the file names
    for (u32 e = 0; e < kEmulatorCount; e++)
    {
        _paths[e][0] = 0;
        for (u32 folder = 0; folder < kSearchFolderCount && _paths[e][0] == 0; folder++)
        {
            for (u32 n = 0; n < 3 && kEmulators[e].fileNames[n]; n++)
            {
                if (!(found[e][n] & (1 << folder)))
                    continue;
                const char* folderPath = kSearchFolders[folder];
                // the root is "/", every other folder needs a slash before the file name
                const char* separator = folderPath[1] == 0 ? "" : "/";
                strlcpy(_paths[e], folderPath, kMaxPathLength);
                strlcat(_paths[e], separator, kMaxPathLength);
                strlcat(_paths[e], kEmulators[e].fileNames[n], kMaxPathLength);
                LOG_INFO("DsRunner: found %s at %s\n", kEmulators[e].name, _paths[e]);
                break;
            }
        }
    }
}

const char* DsRunnerEmulatorFinder::FindForConsole(const char* consoleFolder) const
{
    if (!consoleFolder)
        return nullptr;
    for (u32 e = 0; e < kEmulatorCount; e++)
    {
        if (_paths[e][0] == 0)
            continue;
        for (u32 c = 0; c < 3 && kEmulators[e].consoles[c]; c++)
        {
            if (!strcasecmp(kEmulators[e].consoles[c], consoleFolder))
                return _paths[e];
        }
    }
    return nullptr;
}

#include "common.h"
#include <memory>
#include <string.h>
#include "fat/ff.h"
#include "DsRunnerInGameReset.h"

static bool fileExists(const char* path)
{
    FILINFO fileInfo;
    return path && path[0] != 0 && f_stat(path, &fileInfo) == FR_OK && !(fileInfo.fattrib & AM_DIR);
}

const char* DsRunnerInGameReset::ChooseLauncherPath(int argc, char* argv[])
{
    // Unclear whether the M3 autoboot chain (r4i.sys) passes argv[0], so it is checked.
    if (argc >= 1 && argv[0])
    {
        if (fileExists(argv[0]))
        {
            LOG_DEBUG("Launcher path from argv[0]: %s\n", argv[0]);
            return argv[0];
        }
        LOG_WARNING("argv[0] is not a file on the card: \"%s\"\n", argv[0]);
    }
    else
    {
        LOG_WARNING("No argv[0]\n");
    }

    for (auto path : kFallbackLauncherPaths)
    {
        if (fileExists(path))
        {
            LOG_DEBUG("Launcher path from fallback: %s\n", path);
            return path;
        }
    }

    LOG_ERROR("No launcher path found, Pico Loader can't return to DsRunner\n");
    return nullptr;
}

bool DsRunnerInGameReset::IsInGameResetBoot(int argc, char* argv[])
{
    return argc >= 2 && argv[1] && strcmp(argv[1], kLauncherArgument) == 0;
}

void DsRunnerInGameReset::ApplySetting(bool inGameResetEnabled)
{
    bool disableFileExists = fileExists(kDisableFilePath);
    if (inGameResetEnabled && disableFileExists)
    {
        FRESULT result = f_unlink(kDisableFilePath);
        if (result != FR_OK)
            LOG_ERROR("Couldn't delete %s: %d\n", kDisableFilePath, result);
    }
    else if (!inGameResetEnabled && !disableFileExists)
    {
        auto file = std::make_unique<FIL>();
        FRESULT result = f_open(file.get(), kDisableFilePath, FA_CREATE_NEW | FA_WRITE);
        if (result == FR_OK)
        {
            static const char sText[] = "In-game reset off (DsRunner settings.json \"inGameReset\": false)\r\n";
            UINT bytesWritten;
            f_write(file.get(), sText, sizeof(sText) - 1, &bytesWritten);
            f_close(file.get());
        }
        else
        {
            LOG_ERROR("Couldn't create %s: %d\n", kDisableFilePath, result);
        }
    }
}

#include "common.h"
#include <memory>
#include <string.h>
#include "fat/ff.h"
#include "DsRunnerInGameResetKeys.h"
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

/// @brief Writes kKeysFilePath for a mask that is not the default, unless it already holds it.
static void writeKeysFile(u32 mask)
{
    // "0x08C SELECT+START+DOWN\r\n"
    static const char sHexDigits[] = "0123456789ABCDEF";
    char text[80] = "0x";
    text[2] = sHexDigits[(mask >> 8) & 0xF];
    text[3] = sHexDigits[(mask >> 4) & 0xF];
    text[4] = sHexDigits[mask & 0xF];
    text[5] = ' ';
    DsRunnerInGameResetKeys::Format(mask, text + 6, sizeof(text) - 6 - 3);
    strcat(text, "\r\n");
    u32 length = strlen(text);

    auto file = std::make_unique<FIL>();
    if (f_open(file.get(), DsRunnerInGameReset::kKeysFilePath, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        char existing[sizeof(text)];
        UINT bytesRead = 0;
        bool same = f_read(file.get(), existing, sizeof(existing), &bytesRead) == FR_OK &&
            bytesRead == length && memcmp(existing, text, length) == 0;
        f_close(file.get());
        if (same)
            return;
    }

    FRESULT result = f_open(file.get(), DsRunnerInGameReset::kKeysFilePath, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK)
    {
        LOG_ERROR("Couldn't create %s: %d\n", DsRunnerInGameReset::kKeysFilePath, result);
        return;
    }
    UINT bytesWritten;
    f_write(file.get(), text, length, &bytesWritten);
    f_close(file.get());
    LOG_DEBUG("In-game reset keys: %s", text);
}

void DsRunnerInGameReset::ApplySetting(bool inGameResetEnabled, const char* inGameResetKeys)
{
    // keys first: they don't matter while the reset is off, but the file then is already right
    u32 mask = DsRunnerInGameResetKeys::kDefaultMask;
    if (inGameResetKeys && inGameResetKeys[0] != 0)
    {
        uint32_t parsedMask;
        auto parseResult = DsRunnerInGameResetKeys::Parse(inGameResetKeys, parsedMask);
        if (parseResult == DsRunnerInGameResetKeys::ParseResult::Ok)
        {
            mask = parsedMask;
        }
        else if (parseResult == DsRunnerInGameResetKeys::ParseResult::UnknownKey)
        {
            LOG_ERROR("inGameResetKeys \"%s\": unknown key (possible: A B SELECT START RIGHT LEFT UP DOWN R L), using %s\n",
                inGameResetKeys, DsRunnerInGameResetKeys::kDefaultText);
        }
        else
        {
            LOG_ERROR("inGameResetKeys \"%s\": at least %d keys needed, using %s\n",
                inGameResetKeys, (int)DsRunnerInGameResetKeys::kMinimumKeys, DsRunnerInGameResetKeys::kDefaultText);
        }
    }
    if (mask == DsRunnerInGameResetKeys::kDefaultMask)
    {
        if (fileExists(kKeysFilePath))
        {
            FRESULT result = f_unlink(kKeysFilePath);
            if (result != FR_OK)
                LOG_ERROR("Couldn't delete %s: %d\n", kKeysFilePath, result);
        }
    }
    else
    {
        writeKeysFile(mask);
    }

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

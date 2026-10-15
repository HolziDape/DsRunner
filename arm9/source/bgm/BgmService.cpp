#include "common.h"
#include "core/mini-printf.h"
#include "Pcm16FileAudioStream.h"
#include "BcstmAudioStream.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/SdFolderFactory.h"
#include "romBrowser/FileType/NullFileTypeProvider.h"
#include "themes/ThemeInfoFactory.h"
#include "BgmService.h"

// DsRunner: color variants of the DsRunner theme (type DsRunner) without their own bgm folder
// play the music of the dsrunner theme, like they share its console images
static bool isDsRunnerTheme(const char* themeFolderName)
{
    auto themeInfo = ThemeInfoFactory().CreateFromThemeFolder(themeFolderName);
    return themeInfo && themeInfo->GetType() == ThemeType::DsRunner;
}

bool BgmService::StartBgm(const TCHAR* filePath)
{
    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(filePath))
        return false;

    return _audioStreamPlayer->StartPlayback(std::move(stream));
}

void BgmService::StartBgmFromConfig()
{
    // DsRunner: "music": false in settings.json switches the music off, "musicVolume" is 0 to 100
    const auto& appSettings = _appSettingsService.GetAppSettings();
    if (!appSettings.music || appSettings.musicVolume == 0)
    {
        StopBgm();
        return;
    }
    _audioStreamPlayer->SetVolume((appSettings.musicVolume * 127 + 50) / 100);

    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/bgm", appSettings.theme.GetString());
    NullFileTypeProvider fileTypeProvider;
    auto bgmFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath(pathBuffer);
    if ((!bgmFolder || bgmFolder->GetFileCount() == 0) && isDsRunnerTheme(appSettings.theme.GetString()))
        bgmFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/themes/dsrunner/bgm");
    if (!bgmFolder || bgmFolder->GetFileCount() == 0)
    {
        StopBgm();
        return;
    }
    u32 bgmToPlay = _randomGenerator.NextU32(bgmFolder->GetFileCount());
    auto stream = std::make_unique<BcstmAudioStream>();
    if (!stream->Open(bgmFolder->GetFiles()[bgmToPlay]->GetFastFileRef()))
    {
        StopBgm();
        return;
    }

    _audioStreamPlayer->StartPlayback(std::move(stream));
}

void BgmService::StopBgm()
{
    _audioStreamPlayer->StopPlayback();
}

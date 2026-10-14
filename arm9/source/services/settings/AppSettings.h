#pragma once
#include <memory>
#include "core/String.h"
#include "RomBrowserDisplaySettings.h"
#include "FileAssociation.h"

class AppSettings
{
public:
    String<char, 16> language = "english";
    String<char, 64> theme = "dsrunner";
    String<char, 256> lastUsedFilePath = "";
    /// Folder whose subfolders are the consoles. DsRunner always starts here.
    /// Empty: behave like Pico Launcher and start in the folder of the last used file.
    String<char, 256> libraryPath = "/Games";
    /// DsRunner: show the clock in the status bar of the DsRunner theme.
    bool showClock = true;
    /// DsRunner: L+R+Down+B held for two seconds in a game returns to DsRunner (DsRunner Pico Loader).
    bool inGameReset = true;
    /// DsRunner: keys of the in-game reset, e.g. "SELECT+START+DOWN" (DsRunnerInGameResetKeys).
    String<char, 48> inGameResetKeys = "L+R+DOWN+B";
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;
};
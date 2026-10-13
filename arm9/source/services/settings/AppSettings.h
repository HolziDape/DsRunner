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
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;
};
#pragma once

enum class RomBrowserStateTrigger
{
    None,
    Navigate,
    ChangeDisplayMode,
    ShowGameInfo,
    HideGameInfo,
    FolderLoadDone,
    Launch,
    ShowDisplaySettings,
    HideDisplaySettings,
    GotoSettingsScreen,
    // DsRunner: search keyboard
    ShowSearch,
    HideSearch
};

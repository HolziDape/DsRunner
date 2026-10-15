#pragma once

enum class RomBrowserState
{
    Start,
    Browser,
    GameInfo,
    LoadingFolder,
    Launching,
    DisplaySettings,
    GoingToSettingsScreen,
    // DsRunner: the search keyboard is shown (see DsRunnerSearchKeyboardView)
    SearchInput
};

#pragma once
#include "../IRomBrowserController.h"
#include "services/settings/RomBrowserDisplaySettings.h"

/// @brief View model for the display settings screen.
class DisplaySettingsViewModel
{
public:
    explicit DisplaySettingsViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _romBrowserDisplaySettings(_romBrowserController->GetRomBrowserDisplaySettings()) { }

    constexpr RomBrowserLayout GetRomBrowserDisplayMode() const
    {
        return _romBrowserDisplaySettings.layout;
    }

    void SetRomBrowserDisplayMode(RomBrowserLayout romBrowserDisplayMode)
    {
        if (_romBrowserDisplaySettings.layout != romBrowserDisplayMode)
        {
            _romBrowserDisplaySettings.layout = romBrowserDisplayMode;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    constexpr RomBrowserSortMode GetRomBrowserSortMode() const
    {
        return _romBrowserDisplaySettings.sortMode;
    }

    void SetRomBrowserSortMode(RomBrowserSortMode romBrowserSortMode)
    {
        if (_romBrowserDisplaySettings.sortMode != romBrowserSortMode)
        {
            _romBrowserDisplaySettings.sortMode = romBrowserSortMode;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    /// @brief DsRunner: clock in the status bar on/off.
    bool GetShowClock() const
    {
        return _romBrowserController->GetShowClock();
    }

    void SetShowClock(bool showClock)
    {
        _romBrowserController->SetShowClock(showClock);
    }

    void Close()
    {
        _romBrowserController->HideDisplaySettings();
    }

    void GotoSettingsScreen()
    {
        _romBrowserController->GotoSettingsScreen();
    }

private:
    IRomBrowserController* _romBrowserController;
    RomBrowserDisplaySettings _romBrowserDisplaySettings;
};

#pragma once
#include <string.h>
#include "../IRomBrowserController.h"
#include "../Theme/dsrunner/DsRunnerInGameResetKeys.h"
#include "services/settings/RomBrowserDisplaySettings.h"
#include "themes/dsrunner/DsRunnerText.h"

/// @brief View model for the display settings screen.
class DisplaySettingsViewModel
{
public:
    explicit DisplaySettingsViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _romBrowserDisplaySettings(_romBrowserController->GetRomBrowserDisplaySettings())
    {
        RememberCustomInGameResetKeys();
    }

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

    /// @brief DsRunner: return from games with a key combination (settings.json "inGameReset").
    bool GetInGameReset() const
    {
        return _romBrowserController->GetAppSettings().inGameReset;
    }

    void SetInGameReset(bool inGameReset)
    {
        auto& appSettings = _romBrowserController->GetAppSettings();
        if (appSettings.inGameReset != inGameReset)
        {
            appSettings.inGameReset = inGameReset;
            _romBrowserController->SaveSettingsLater();
        }
    }

    /// DsRunner: the key combinations of the display settings ("inGameResetKeys"), the first is the default.
    static constexpr u32 InGameResetKeyPresetCount = 3;
    static constexpr const char* InGameResetKeyPresets[InGameResetKeyPresetCount] =
    {
        DsRunnerInGameResetKeys::kDefaultText,
        "SELECT+START+DOWN",
        "SELECT+DOWN+B",
    };
    /// Index of the custom keys of settings.json in GetInGameResetKeysIndex().
    static constexpr int InGameResetKeysCustom = InGameResetKeyPresetCount;

    /// @brief DsRunner: gets the key combination: the index of a preset, or InGameResetKeysCustom for other
    ///        valid keys of settings.json. Invalid keys count as the default, they give the default keys.
    int GetInGameResetKeysIndex() const
    {
        u32 mask = GetKeyMask(_romBrowserController->GetAppSettings().inGameResetKeys.GetString());
        for (u32 i = 0; i < InGameResetKeyPresetCount; i++)
        {
            if (mask == GetKeyMask(InGameResetKeyPresets[i]))
                return i;
        }
        return InGameResetKeysCustom;
    }

    /// @brief DsRunner: returns true when settings.json had keys that are no preset. They stay selectable
    ///        ("Eigene") until DsRunner restarts.
    bool HasCustomInGameResetKeys() const
    {
        return _customInGameResetKeys[0] != 0;
    }

    void SetInGameResetKeysIndex(int index)
    {
        if (index == GetInGameResetKeysIndex() || index < 0 || index > InGameResetKeysCustom ||
            (index == InGameResetKeysCustom && !HasCustomInGameResetKeys()))
        {
            return;
        }
        _romBrowserController->GetAppSettings().inGameResetKeys =
            index == InGameResetKeysCustom ? _customInGameResetKeys : InGameResetKeyPresets[index];
        _romBrowserController->SaveSettingsLater();
    }

    /// @brief DsRunner: language of the DsRunner UI (settings.json "uiLanguage"). Changes right away.
    bool GetEnglish() const
    {
        return DsRunnerText::IsEnglish();
    }

    void SetEnglish(bool english)
    {
        if (english == DsRunnerText::IsEnglish())
            return;
        DsRunnerText::SetEnglish(english);
        _romBrowserController->GetAppSettings().uiLanguage = english ? "en" : "de";
        _romBrowserController->SaveSettingsLater();
        _romBrowserController->RefreshViews();
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
    // DsRunner: valid keys of settings.json that are no preset, else empty
    char _customInGameResetKeys[48] = { 0 };

    static u32 GetKeyMask(const char* keys)
    {
        u32 mask;
        if (DsRunnerInGameResetKeys::Parse(keys, mask) != DsRunnerInGameResetKeys::ParseResult::Ok)
            mask = DsRunnerInGameResetKeys::kDefaultMask;
        return mask;
    }

    void RememberCustomInGameResetKeys()
    {
        if (GetInGameResetKeysIndex() == InGameResetKeysCustom)
        {
            strlcpy(_customInGameResetKeys, _romBrowserController->GetAppSettings().inGameResetKeys.GetString(),
                sizeof(_customInGameResetKeys));
        }
    }
};

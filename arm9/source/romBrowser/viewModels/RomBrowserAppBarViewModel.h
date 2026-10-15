#pragma once
#include "../IRomBrowserController.h"
#include "themes/dsrunner/DsRunnerText.h"

/// @brief View model for the rom browser app bar
class RomBrowserAppBarViewModel
{
public:
    explicit RomBrowserAppBarViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void NavigateUp()
    {
        _romBrowserController->NavigateUp();
    }

    void ShowDisplaySettings()
    {
        _romBrowserController->ShowDisplaySettings();
    }

    /// @brief Gets the title for the app bar: the console level has its own title,
    ///        other folders are named after the folder.
    const char* GetTitle() const
    {
        if (_romBrowserController->IsCurrentFolderLibraryRoot())
        {
            return DsRunnerText::Get(DsRunnerText::Id::AppBarConsoles);
        }
        const char* name = _romBrowserController->GetCurrentFolderName();
        if (name[0] == 0)
            return DsRunnerText::Get(DsRunnerText::Id::AppBarSdCard);
        // DsRunner: the favorites folder is named "★ Favoriten" inside, Localize translates it
        return _romBrowserController->IsCurrentFolderVirtual() ? DsRunnerText::Localize(name) : name;
    }

    constexpr RomBrowserLayout GetRomBrowserLayout() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().layout;
    }

private:
    IRomBrowserController* _romBrowserController;
};

#pragma once
#include "../IRomBrowserController.h"

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
            return "Konsolen";
        }
        const char* name = _romBrowserController->GetCurrentFolderName();
        return name[0] != 0 ? name : "SD-Karte";
    }

    constexpr RomBrowserLayout GetRomBrowserLayout() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().layout;
    }

private:
    IRomBrowserController* _romBrowserController;
};

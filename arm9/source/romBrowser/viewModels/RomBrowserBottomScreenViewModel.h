#pragma once
#include "core/SharedPtr.h"
#include "../RomBrowserController.h"
#include "RomBrowserViewModel.h"
#include "RomBrowserAppBarViewModel.h"

class ICoverRepository;

/// @brief View model for the rom browser bottom screen
class RomBrowserBottomScreenViewModel
{
public:
    RomBrowserBottomScreenViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _romBrowserAppBarViewModel(romBrowserController) { }

    bool IsRomBrowserVisible() const
    {
        auto state = _romBrowserController->GetStateMachine().GetCurrentState();
        return state == RomBrowserState::Browser
            || state == RomBrowserState::GameInfo
            || state == RomBrowserState::DisplaySettings
            || state == RomBrowserState::Launching
            || state == RomBrowserState::GoingToSettingsScreen;
    }

    const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel()
    {
        return _romBrowserController->GetRomBrowserViewModel();
    }

    RomBrowserAppBarViewModel* GetRomBrowserAppBarViewModel()
    {
        return &_romBrowserAppBarViewModel;
    }

    void NavigateUp()
    {
        _romBrowserController->NavigateUp();
    }

    /// @brief DsRunner: adds the selected game to the favorites or removes it.
    void ToggleFavorite()
    {
        const auto& romBrowserViewModel = _romBrowserController->GetRomBrowserViewModel();
        if (!romBrowserViewModel.IsValid())
            return;
        int selectedItem = romBrowserViewModel->GetSelectedItem();
        auto& fileInfoManager = romBrowserViewModel->GetFileInfoManager();
        if (selectedItem >= 0 && (u32)selectedItem < fileInfoManager.GetItemCount())
            _romBrowserController->ToggleFavorite(fileInfoManager.GetItem(selectedItem));
    }

    /// @brief DsRunner: starts the last played game ("Weiterspielen", console level only).
    bool ContinueLastGame()
    {
        return _romBrowserController->ContinueLastGame();
    }

private:
    IRomBrowserController* _romBrowserController;
    RomBrowserAppBarViewModel _romBrowserAppBarViewModel;
};

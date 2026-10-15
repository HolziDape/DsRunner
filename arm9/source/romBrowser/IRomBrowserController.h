#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class IIconRepository;
class IBannerRepository;
class ICheatRepository;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    virtual void LaunchFile(const FileInfo& fileInfo) = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;
    virtual void GotoSettingsScreen() = 0;

    virtual void Update() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;
    virtual const IIconRepository& GetIconRepository() const = 0;
    virtual const IBannerRepository& GetBannerRepository() const = 0;
    virtual const ICheatRepository& GetCheatRepository() const = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;

    /// @brief Gets the name of the folder that is currently shown, or an empty string for the sd root.
    virtual const TCHAR* GetCurrentFolderName() const = 0;

    /// @brief Returns true when the folder that is currently shown is the library root,
    ///        the folder whose subfolders are the consoles.
    virtual bool IsCurrentFolderLibraryRoot() const = 0;

    /// @brief Gets the full path of the folder that is currently shown (may start with a drive like "fat:").
    virtual const TCHAR* GetCurrentFolderPath() const = 0;

    /// @brief DsRunner: gets the folder that contains a shown item. This is the current folder,
    ///        except in the favorites list, which shows files of several folders.
    virtual const TCHAR* GetItemFolderPath(const FileInfo& fileInfo) const = 0;

    /// @brief DsRunner: adds a file to the favorites or removes it (folders are ignored).
    virtual void ToggleFavorite(const FileInfo& fileInfo) = 0;

    /// @brief DsRunner: starts the last played game ("Weiterspielen"). Only works on the console level.
    /// @return True when a game is started.
    virtual bool ContinueLastGame() = 0;

    /// @brief DsRunner: shows or hides the clock of the status bar (saved when the display settings close).
    virtual void SetShowClock(bool showClock) = 0;
    virtual bool GetShowClock() const = 0;

    /// @brief DsRunner: opens the search keyboard (DsRunnerSearchKeyboardView).
    /// @return True when it opens.
    virtual bool OpenSearch() = 0;

    /// @brief DsRunner: closes the search keyboard. When it was opened from the search results,
    ///        this goes back to the console level.
    virtual void CloseSearch() = 0;

    /// @brief DsRunner: shows the games that match \p query as a folder (from the search keyboard).
    virtual void StartSearch(const char* query) = 0;

    /// @brief DsRunner: shows the matches of the next (1) or previous (-1) console in the search results.
    /// @return True when the search results are shown (the key is used).
    virtual bool SwitchSearchFilter(int direction) = 0;

    /// @brief DsRunner: the query of the search results, also the one the keyboard starts with.
    virtual const char* GetSearchQuery() const = 0;

    /// @brief DsRunner: returns true while the search results are shown.
    virtual bool IsCurrentFolderSearch() const = 0;

    /// @brief DsRunner: returns true for the favorites and the search results, which show files of several folders.
    virtual bool IsCurrentFolderVirtual() const = 0;

    /// @brief DsRunner: gets the group a shown item is sorted by first: its console in the search results, else 0.
    virtual int GetItemGroup(const FileInfo& fileInfo) const = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }

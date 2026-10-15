#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "SdFolder.h"
#include "viewModels/RomBrowserViewModel.h"
#include "RomBrowserStateMachine.h"
#include "core/task/TaskQueue.h"
#include "IRomBrowserController.h"
#include "CoverRepository.h"
#include "IconRepository.h"
#include "BannerRepository.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/ICheatRepository.h"
#include "Theme/dsrunner/DsRunnerLibrary.h"
#include "Theme/dsrunner/DsRunnerSearch.h"

class RomBrowserController : public IRomBrowserController
{
public:
    RomBrowserController(IAppSettingsService* appSettingsService,
        TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue);

    void NavigateUp() override;

    void NavigateToPath(const TCHAR* name) override;
    void LaunchFile(const FileInfo& fileInfo) override;
    void ShowGameInfo(const FileInfo& fileInfo) override;
    void HideGameInfo() override;
    void ShowDisplaySettings() override;
    void HideDisplaySettings() override;
    void GotoSettingsScreen() override;

    void Update() override;

    const SdFolder& GetSdFolder() const override { return *_sdFolder; }

    const RomBrowserStateMachine& GetStateMachine() const override { return _stateMachine; }

    const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() override { return _romBrowserViewModel; }

    TaskQueueBase* GetIoTaskQueue() const override { return _ioTaskQueue; }
    TaskQueueBase* GetBgTaskQueue() const override { return _bgTaskQueue; }
    const ICoverRepository& GetCoverRepository() const override { return *_coverRepository; }
    const IIconRepository& GetIconRepository() const override { return *_iconRepository; }
    const IBannerRepository& GetBannerRepository() const override { return *_bannerRepository; }
    const ICheatRepository& GetCheatRepository() const override { return *_cheatRepository; }

    void SetRomBrowserDisplaySettings(const RomBrowserDisplaySettings& romBrowserDisplaySettings) override;

    const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const override
    {
        return _appSettingsService->GetAppSettings().romBrowserDisplaySettings;
    }

    virtual const FileInfo& GetTriggerFileInfo() const override { return _triggerFileInfo; }

    const TCHAR* GetCurrentFolderName() const override { return _currentFolderName; }
    bool IsCurrentFolderLibraryRoot() const override { return _currentFolderIsLibraryRoot; }
    const TCHAR* GetCurrentFolderPath() const override { return _currentFolderPath; }

    // DsRunner: favorites, "Weiterspielen" and options of the DsRunner theme
    const TCHAR* GetItemFolderPath(const FileInfo& fileInfo) const override;
    void ToggleFavorite(const FileInfo& fileInfo) override;
    bool ContinueLastGame() override;
    void SetShowClock(bool showClock) override;
    bool GetShowClock() const override { return _appSettingsService->GetAppSettings().showClock; }
    AppSettings& GetAppSettings() override { return _appSettingsService->GetAppSettings(); }
    void SaveSettingsLater() override { _saveSettingsPending = true; }
    void RefreshViews() override;

    // DsRunner: search over all games (see DsRunnerSearch)
    bool OpenSearch() override;
    void CloseSearch() override;
    void StartSearch(const char* query) override;
    bool SwitchSearchFilter(int direction) override;
    const char* GetSearchQuery() const override { return _searchQuery; }
    bool IsCurrentFolderSearch() const override { return _currentFolderIsSearch; }
    bool IsCurrentFolderVirtual() const override { return _currentFolderIsSearch || _currentFolderIsFavorites; }
    int GetItemGroup(const FileInfo& fileInfo) const override
    {
        return _currentFolderIsSearch && _searchResults ? _searchResults->GetGroup(&fileInfo) : 0;
    }

private:
    IAppSettingsService* _appSettingsService;
    TaskQueueBase* _ioTaskQueue;
    TaskQueueBase* _bgTaskQueue;

    std::unique_ptr<SdFolder> _sdFolder;
    SharedPtr<RomBrowserViewModel> _romBrowserViewModel;
    std::unique_ptr<SdFolder> _newSdFolder;
    RomBrowserStateMachine _stateMachine;
    TCHAR _navigatePath[256];
    TCHAR* _navigateFileName;
    // entry to select after navigating to a folder (the console of the last played game)
    TCHAR _navigateSelect[256] = { 0 };
    // info about the folder being loaded, filled by the io thread and taken over when loading is done
    TCHAR _newFolderName[256] = { 0 };
    bool _newFolderIsLibraryRoot = false;
    TCHAR _newFolderPath[256] = { 0 };
    TCHAR _currentFolderName[256] = { 0 };
    TCHAR _currentFolderPath[256] = { 0 };
    bool _currentFolderIsLibraryRoot = false;
    // DsRunner: the virtual favorites folder (see DsRunnerFavoritesFolder)
    bool _navigateToFavorites = false;
    bool _newFolderIsFavorites = false;
    bool _currentFolderIsFavorites = false;
    std::unique_ptr<DsRunnerFavoritesFolder> _newFavoritesFolder;
    std::unique_ptr<DsRunnerFavoritesFolder> _favoritesFolder;
    // DsRunner: the virtual folder with the search results (see DsRunnerSearch)
    bool _navigateToSearch = false;
    bool _newFolderIsSearch = false;
    bool _currentFolderIsSearch = false;
    std::unique_ptr<DsRunnerSearchResults> _newSearchResults;
    std::unique_ptr<DsRunnerSearchResults> _searchResults;
    char _searchQuery[DsRunnerSearch::MaxQueryLength + 1] = { 0 };
    // console group shown in the search results, -1 for all
    int _searchGroup = -1;
    QueueTask<void> _searchIndexTask;
    // DsRunner: launch the selected file once the folder is loaded ("Weiterspielen")
    bool _launchAfterNavigate = false;
    // DsRunner: the game START continues, found while loading the console level
    TCHAR _newContinuePath[256] = { 0 };
    // DsRunner: folder of the file being launched (for files of the favorites folder)
    TCHAR _launchFolderPath[256] = { 0 };
    FileInfo _triggerFileInfo;
    QueueTask<void> _navigateTask;
    QueueTask<void> _gameCountTask;
    bool _saveSettingsPending = false;
    // DsRunner: item to select when the views are built again (RefreshViews), empty for the first one
    TCHAR _refreshSelect[256] = { 0 };
    std::unique_ptr<CoverRepository> _coverRepository;
    std::unique_ptr<IconRepository> _iconRepository;
    std::unique_ptr<BannerRepository> _bannerRepository;
    ExtensionFileTypeProvider _fileTypeProvider;
    std::unique_ptr<ICheatRepository> _cheatRepository;

    void HandleTrigger();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void HandleLaunchTrigger();
    void HandleChangeDisplayModeTrigger();
    void HandleGotoSettingsScreenTrigger();
    void StartGameCount();
    void StartSearchIndex();
    void NavigateToSearch();
    void MakeSearchTitle();
    void FindContinueGame();
    void UpdateLastUsedFilepath();
    void SetPicoLoaderParams() const;
    void LoadCheats() const;
};

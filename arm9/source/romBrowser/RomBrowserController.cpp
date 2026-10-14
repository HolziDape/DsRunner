#include "common.h"
#include <array>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "Theme/dsrunner/DsRunnerLibrary.h"
#include "Theme/dsrunner/DsRunnerSystems.h"
#include "Theme/dsrunner/DsRunnerInGameReset.h"
#include "RomBrowserController.h"

// Skips a drive prefix like "fat:" and the leading slashes of a path.
static const char* skipDriveAndSlashes(const char* path)
{
    const char* colon = strchr(path, ':');
    if (colon)
        path = colon + 1;
    while (*path == '/')
        path++;
    return path;
}

// Compares two paths ignoring case, drive prefix and leading/trailing slashes.
static bool isSamePath(const char* a, const char* b)
{
    a = skipDriveAndSlashes(a);
    b = skipDriveAndSlashes(b);
    size_t lengthA = strlen(a);
    size_t lengthB = strlen(b);
    while (lengthA > 0 && a[lengthA - 1] == '/')
        lengthA--;
    while (lengthB > 0 && b[lengthB - 1] == '/')
        lengthB--;
    return lengthA == lengthB && strncasecmp(a, b, lengthA) == 0;
}

// Copies the first path component of \p path below \p basePath to \p result,
// or an empty string when \p path is not below \p basePath.
static void getFirstComponentBelow(const char* basePath, const char* path, char* result, size_t resultSize)
{
    result[0] = 0;
    basePath = skipDriveAndSlashes(basePath);
    path = skipDriveAndSlashes(path);
    size_t baseLength = strlen(basePath);
    while (baseLength > 0 && basePath[baseLength - 1] == '/')
        baseLength--;
    if (strncasecmp(path, basePath, baseLength) != 0 || (path[baseLength] != '/' && baseLength != 0))
        return;
    const char* rest = path + baseLength;
    while (*rest == '/')
        rest++;
    size_t length = 0;
    while (rest[length] != 0 && rest[length] != '/')
        length++;
    if (length == 0 || length >= resultSize)
        return;
    memcpy(result, rest, length);
    result[length] = 0;
}

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, TaskQueueBase* ioTaskQueue,
    TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
    , _ioTaskQueue(ioTaskQueue), _bgTaskQueue(bgTaskQueue)
    , _fileTypeProvider(appSettingsService->GetAppSettings())
{
    gDsRunnerLibrary.appSettings = &appSettingsService->GetAppSettings();
}

void RomBrowserController::NavigateUp()
{
    if (_currentFolderIsLibraryRoot)
    {
        // the library root is the top level of the launcher
        return;
    }
    if (_currentFolderIsFavorites)
    {
        // DsRunner: the favorites folder is virtual, it belongs to the library root
        NavigateToPath(_appSettingsService->GetAppSettings().libraryPath.GetString());
        StringUtil::Copy(_navigateSelect, DsRunnerFavoritesFolder::EntryName,
            sizeof(_navigateSelect) / sizeof(_navigateSelect[0]));
        return;
    }
    // select the folder we come from in its parent
    TCHAR leavingFolder[256];
    StringUtil::Copy(leavingFolder, _currentFolderName, sizeof(leavingFolder) / sizeof(leavingFolder[0]));
    NavigateToPath("..");
    StringUtil::Copy(_navigateSelect, leavingFolder, sizeof(_navigateSelect) / sizeof(_navigateSelect[0]));
}

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _navigateSelect[0] = 0;
    // DsRunner: the favorites tile of the console level opens the virtual favorites folder
    _navigateToFavorites = _currentFolderIsLibraryRoot && DsRunnerFavoritesFolder::IsEntry(name);
    _launchAfterNavigate = false;
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    // DsRunner: files in the favorites folder are not in the current directory
    StringUtil::Copy(_launchFolderPath, GetItemFolderPath(fileInfo),
        sizeof(_launchFolderPath) / sizeof(_launchFolderPath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

const TCHAR* RomBrowserController::GetItemFolderPath(const FileInfo& fileInfo) const
{
    if (_currentFolderIsFavorites && _favoritesFolder)
    {
        const char* folderPath = _favoritesFolder->GetFolderPath(&fileInfo);
        if (folderPath)
            return folderPath;
    }
    return _currentFolderPath;
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo)
{
    if (fileInfo.GetFileType()->GetClassification() == FileTypeClassification::Folder)
        return;
    gDsRunnerLibrary.favorites.Toggle(GetItemFolderPath(fileInfo), fileInfo.GetFileName(), _ioTaskQueue);
}

bool RomBrowserController::ContinueLastGame()
{
    if (!_currentFolderIsLibraryRoot || gDsRunnerLibrary.continuePath[0] == 0 ||
        _stateMachine.GetCurrentState() != RomBrowserState::Browser)
    {
        return false;
    }
    // open the folder of the game with the game selected, then launch it the normal way
    NavigateToPath(gDsRunnerLibrary.continuePath);
    _launchAfterNavigate = true;
    return true;
}

void RomBrowserController::SetShowClock(bool showClock)
{
    auto& appSettings = _appSettingsService->GetAppSettings();
    if (appSettings.showClock != showClock)
    {
        appSettings.showClock = showClock;
        _saveSettingsPending = true;
    }
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfo);
}

void RomBrowserController::HideGameInfo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfo);
}

void RomBrowserController::ShowDisplaySettings()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowDisplaySettings);
}

void RomBrowserController::HideDisplaySettings()
{
    if (_saveSettingsPending)
    {
        _saveSettingsPending = false;
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _appSettingsService->Save();
            return TaskResult<void>::Completed();
        });
    }
    _stateMachine.Fire(RomBrowserStateTrigger::HideDisplaySettings);
}

void RomBrowserController::GotoSettingsScreen()
{
    _stateMachine.Fire(RomBrowserStateTrigger::GotoSettingsScreen);
}

void RomBrowserController::SetRomBrowserDisplaySettings(
    const RomBrowserDisplaySettings& romBrowserDisplaySettings)
{
    _appSettingsService->GetAppSettings().romBrowserDisplaySettings = romBrowserDisplaySettings;
    _saveSettingsPending = true;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::Update()
{
    _stateMachine.Update();
    if (_stateMachine.HasStateChanged())
    {
        HandleTrigger();
    }
    switch (_stateMachine.GetCurrentState())
    {
        case RomBrowserState::Start:
        {
            LOG_DEBUG("RomBrowserState::Start\n");
            const auto& appSettings = _appSettingsService->GetAppSettings();
            const char* libraryPath = appSettings.libraryPath.GetString();
            const char* lastUsed = appSettings.lastUsedFilePath.GetString();
            if (strlen(libraryPath) != 0)
            {
                // always start on the library root (the console level). The console of the
                // game that was played last is selected. When the library folder doesn't
                // exist the navigation falls back to the sd root.
                NavigateToPath(libraryPath);
                getFirstComponentBelow(libraryPath, lastUsed, _navigateSelect,
                    sizeof(_navigateSelect) / sizeof(_navigateSelect[0]));
            }
            else if (strlen(lastUsed) != 0)
            {
                NavigateToPath(lastUsed);
            }
            else
            {
                NavigateToPath("/");
            }
            break;
        }
        case RomBrowserState::LoadingFolder:
        {
            if (_navigateTask.GetTask().IsCompletedSuccessfully())
            {
                _navigateTask.Dispose();
                _stateMachine.Fire(RomBrowserStateTrigger::FolderLoadDone);
            }
            break;
        }
        case RomBrowserState::Launching:
        default:
        {
            break;
        }
    }
}

void RomBrowserController::HandleTrigger()
{
    switch (_stateMachine.GetLastTrigger())
    {
        case RomBrowserStateTrigger::Navigate:
            HandleNavigateTrigger();
            break;

        case RomBrowserStateTrigger::FolderLoadDone:
            HandleFolderLoadDoneTrigger();
            break;

        case RomBrowserStateTrigger::Launch:
            HandleLaunchTrigger();
            break;

        case RomBrowserStateTrigger::ChangeDisplayMode:
            HandleChangeDisplayModeTrigger();
            break;

        case RomBrowserStateTrigger::GotoSettingsScreen:
            HandleGotoSettingsScreenTrigger();
            break;

        default:
            break;
    }
}

void RomBrowserController::HandleNavigateTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Navigate\n");
    _navigateTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!_coverRepository)
        {
            _coverRepository = std::make_unique<CoverRepository>();
            _coverRepository->Initialize();
        }
        if (!_iconRepository)
        {
            _iconRepository = std::make_unique<IconRepository>();
            _iconRepository->Initialize();
        }
        if (!_bannerRepository)
        {
            _bannerRepository = std::make_unique<BannerRepository>();
            _bannerRepository->Initialize();
        }
        if (!_cheatRepository)
        {
            _cheatRepository = UsrCheatRepositoryFactory().FromUsrCheatDat("/_pico/usrcheat.dat");
            if (!_cheatRepository)
            {
                // When usrcheat.dat is not found or cannot be read use a dummy empty cheat repository
                _cheatRepository = std::make_unique<EmptyCheatRepository>();
            }
        }
        // DsRunner: consoles of /_dsrunner/systems.json and emulators on the sd card, once (before file types are needed)
        gDsRunnerSystems.Load(_appSettingsService->GetAppSettings());
        // DsRunner: the play history is needed for sorting by last played
        gDsRunnerLibrary.playHistory.Load();
        gDsRunnerLibrary.favorites.Load();
        _newContinuePath[0] = 0;

        u64 startTick = gTickCounter.GetValue();
        _newFavoritesFolder.reset();
        _newFolderIsFavorites = false;
        if (_navigateToFavorites)
        {
            // DsRunner: the virtual favorites folder. The current directory stays the library root.
            _newFavoritesFolder = DsRunnerFavoritesFolder::Create(
                gDsRunnerLibrary.favorites, _fileTypeProvider, _newSdFolder);
            _newFolderIsFavorites = true;
            _newFolderIsLibraryRoot = false;
            StringUtil::Copy(_newFolderName, DsRunnerFavoritesFolder::EntryName,
                sizeof(_newFolderName) / sizeof(_newFolderName[0]));
            if (!DsRunnerPaths::Combine(_appSettingsService->GetAppSettings().libraryPath.GetString(),
                    DsRunnerFavoritesFolder::EntryName, _newFolderPath, sizeof(_newFolderPath)))
            {
                _newFolderPath[0] = 0;
            }
            _navigateFileName = _navigateSelect[0] != 0 ? _navigateSelect : nullptr;
            return TaskResult<void>::Completed();
        }
        _navigateFileName = nullptr;
        if (strcmp(_navigatePath, "/") != 0) // can't f_stat on root dir
        {
            FILINFO fileInfo;
            if (f_stat(_navigatePath, &fileInfo) != FR_OK)
            {
                StringUtil::Copy(_navigatePath, "/", sizeof(_navigatePath) / sizeof(_navigatePath[0]));
            }
            else if (!(fileInfo.fattrib & AM_DIR))
            {
                _navigateFileName = strrchr(_navigatePath, '/') + 1;
                _navigateFileName[-1] = 0;
            }
        }
        f_chdir(_navigatePath);
        if (_navigateFileName == nullptr && _navigateSelect[0] != 0)
        {
            _navigateFileName = _navigateSelect;
        }

        // remember where we are for the app bar title and the console level detection
        TCHAR currentPath[256];
        if (f_getcwd(currentPath, sizeof(currentPath) / sizeof(currentPath[0])) != FR_OK)
        {
            currentPath[0] = 0;
        }
        const char* name = strrchr(currentPath, '/');
        name = name ? name + 1 : currentPath;
        if (strchr(name, ':'))
        {
            // the root of a drive ("fat:"), not a real folder name
            name = "";
        }
        StringUtil::Copy(_newFolderName, name, sizeof(_newFolderName) / sizeof(_newFolderName[0]));
        StringUtil::Copy(_newFolderPath, currentPath, sizeof(_newFolderPath) / sizeof(_newFolderPath[0]));
        const char* libraryPath = _appSettingsService->GetAppSettings().libraryPath.GetString();
        _newFolderIsLibraryRoot = libraryPath[0] != 0 && isSamePath(currentPath, libraryPath);

        SdFolderFactory sdFolderFactory { &_fileTypeProvider };
        _newSdFolder = sdFolderFactory.CreateFromPath(".");
        if (_newFolderIsLibraryRoot && _newSdFolder)
        {
            // DsRunner: the console level gets the favorites tile and knows the game START continues
            DsRunnerFavoritesFolder::AddEntry(*_newSdFolder, gDsRunnerLibrary.favorites);
            FindContinueGame();
        }
        u64 endTick = gTickCounter.GetValue();
        LOG_DEBUG("Loading files in folder took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleFolderLoadDoneTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::FolderLoadDone\n");
    _romBrowserViewModel.Reset();
    _sdFolder = std::move(_newSdFolder);
    StringUtil::Copy(_currentFolderName, _newFolderName, sizeof(_currentFolderName) / sizeof(_currentFolderName[0]));
    StringUtil::Copy(_currentFolderPath, _newFolderPath, sizeof(_currentFolderPath) / sizeof(_currentFolderPath[0]));
    _currentFolderIsLibraryRoot = _newFolderIsLibraryRoot;
    _currentFolderIsFavorites = _newFolderIsFavorites;
    _favoritesFolder = std::move(_newFavoritesFolder);
    if (_currentFolderIsLibraryRoot)
    {
        StringUtil::Copy(gDsRunnerLibrary.continuePath, _newContinuePath,
            sizeof(gDsRunnerLibrary.continuePath) / sizeof(gDsRunnerLibrary.continuePath[0]));
    }
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this, _navigateFileName);
    StartGameCount();

    if (_launchAfterNavigate)
    {
        // DsRunner "Weiterspielen": the game is selected in its folder, launch it like with A
        _launchAfterNavigate = false;
        int index = _romBrowserViewModel->GetSelectedItem();
        if (index >= 0)
        {
            const auto& item = _romBrowserViewModel->GetFileInfoManager().GetItem(index);
            if (item.GetFileType()->GetClassification() != FileTypeClassification::Folder)
                LaunchFile(item);
        }
    }
}

void RomBrowserController::FindContinueGame()
{
    // the newest entry of the play history that still exists, or the last used file
    _newContinuePath[0] = 0;
    const char* candidates[4];
    u32 candidateCount = 0;
    for (u32 i = 0; i < 3 && i < gDsRunnerLibrary.playHistory.GetCount(); i++)
        candidates[candidateCount++] = gDsRunnerLibrary.playHistory.Get(i);
    candidates[candidateCount++] = _appSettingsService->GetAppSettings().lastUsedFilePath.GetString();
    for (u32 i = 0; i < candidateCount; i++)
    {
        FILINFO fileInfo;
        if (candidates[i][0] != 0 && f_stat(candidates[i], &fileInfo) == FR_OK && !(fileInfo.fattrib & AM_DIR))
        {
            StringUtil::Copy(_newContinuePath, candidates[i],
                sizeof(_newContinuePath) / sizeof(_newContinuePath[0]));
            return;
        }
    }
}

void RomBrowserController::StartGameCount()
{
    // DsRunner: count the games of the consoles in the background, once
    auto& gameCounts = gDsRunnerLibrary.gameCounts;
    if (!_currentFolderIsLibraryRoot || !_sdFolder ||
        (_gameCountTask.IsValid() && !_gameCountTask.GetTask().IsCompleted()) ||
        !gameCounts.NeedsCounting(_currentFolderPath))
    {
        return;
    }
    gameCounts.SetFolders(_currentFolderPath, *_sdFolder);
    _gameCountTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        gDsRunnerLibrary.gameCounts.Count(_fileTypeProvider, cancelRequested);
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    // don't let the game count delay the launch
    _gameCountTask.CancelTask();
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        UpdateLastUsedFilepath();
        SetPicoLoaderParams();
        LoadCheats();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleChangeDisplayModeTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::ChangeDisplayMode\n");
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this);
}

void RomBrowserController::HandleGotoSettingsScreenTrigger()
{
    _gameCountTask.CancelTask();
    gProcessManager.Goto<SettingsProcess>();
}

void RomBrowserController::UpdateLastUsedFilepath()
{
    f_getcwd(_navigatePath, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    if (_currentFolderIsFavorites)
    {
        // DsRunner: files of the favorites folder are not in the current directory. Keep its drive ("fat:").
        char* colon = strchr(_navigatePath, ':');
        u32 driveLength = colon ? colon + 1 - _navigatePath : 0;
        DsRunnerPaths::Combine(_launchFolderPath, _triggerFileInfo.GetFileName(),
            _navigatePath + driveLength, sizeof(_navigatePath) - driveLength);
    }
    else
    {
        int idx = strlcat(_navigatePath, "/", sizeof(_navigatePath));
        if (_navigatePath[idx - 2] == '/')
        {
            _navigatePath[idx - 1] = 0;
        }
        strlcat(_navigatePath, _triggerFileInfo.GetFileName(), sizeof(_navigatePath));
    }
    _appSettingsService->GetAppSettings().lastUsedFilePath = _navigatePath;
    _appSettingsService->Save();
    gDsRunnerLibrary.playHistory.Add(_navigatePath);
}

void RomBrowserController::SetPicoLoaderParams() const
{
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;
    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        // DsRunner: switch the in-game reset of the DsRunner Pico Loader on or off (settings.json "inGameReset")
        DsRunnerInGameReset::ApplySetting(_appSettingsService->GetAppSettings().inGameReset);
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}

#pragma once
#include <memory>
#include "common.h"
#include "core/StringUtil.h"
#include "romBrowser/FileType/FileType.h"
#include "romBrowser/Theme/IThemeFileIconFactory.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "DsRunnerEmulators.h"

class AppSettings;

/// @brief File type of a rom that is started with an emulator. Like Pico's CustomFileType, but the
///        emulator comes from /_dsrunner/systems.json or was found on the sd card (DsRunnerEmulators).
class DsRunnerEmulatorFileType : public FileType
{
public:
    DsRunnerEmulatorFileType()
        : FileType(nullptr, FileTypeClassification::Unknown) { }

    /// @param extension The extension (without dot), must stay valid.
    /// @param emulatorPath The emulator that gets the rom as argument, must stay valid.
    /// @param baseFileType File type for icon and header (GBA), or nullptr.
    DsRunnerEmulatorFileType(const char* extension, const char* emulatorPath, const FileType* baseFileType)
        : FileType(baseFileType ? baseFileType->GetShortName() : extension, FileTypeClassification::Game)
        , _extension(extension), _emulatorPath(emulatorPath), _baseFileType(baseFileType) { }

    const char* GetExtension() const { return _extension; }

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
    {
        // without an icon of its own the DsRunner theme draws the console badge (DsRunnerConsoles::FindByFileName)
        return _baseFileType != nullptr
            ? _baseFileType->CreateFileIcon(fileName, themeFileIconFactory)
            : themeFileIconFactory->CreateGenericFileIcon(fileName);
    }

    FileCover* CreateFileCover(const TCHAR* fileName) const override
    {
        return _baseFileType != nullptr
            ? _baseFileType->CreateFileCover(fileName)
            : FileType::CreateFileCover(fileName);
    }

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
    {
        return _baseFileType != nullptr
            ? _baseFileType->CreateInternalFileInfo(fastFileRef)
            : nullptr;
    }

    bool TrySetLaunchParameters(pload_params_t* launchParameters, const char* filePath) const override
    {
        // same as CustomFileType: start the emulator, the rom path is its argument
        StringUtil::Copy(launchParameters->romPath, _emulatorPath, sizeof(launchParameters->romPath));
        u32 length = StringUtil::Copy(launchParameters->arguments, filePath, sizeof(launchParameters->arguments));
        launchParameters->argumentsLength = length + 1;
        return true;
    }

private:
    const char* _extension = nullptr;
    const char* _emulatorPath = nullptr;
    const FileType* _baseFileType = nullptr;
};

/// @brief The consoles of /_dsrunner/systems.json and the emulators for all consoles.
///
/// Priority for starting a rom with an extension:
/// 1. fileAssociations in /_pico/settings.json (checked by ExtensionFileTypeProvider before this class)
/// 2. "emulator" of a console in /_dsrunner/systems.json
/// 3. an emulator of DsRunnerEmulators found on the sd card
/// Files without any of these are not shown (Pico can't start them).
class DsRunnerSystems
{
public:
    static constexpr const char* FilePath = "/_dsrunner/systems.json";
    /// Larger files are ignored.
    static constexpr u32 MaxFileSize = 8 * 1024;
    /// At most this many consoles are taken from systems.json, the rest is ignored.
    static constexpr u32 MaxCustomConsoles = 16;
    /// At most this many file extensions per console.
    static constexpr u32 MaxExtensions = 4;

    /// @brief Reads systems.json and looks for the emulators. Only the first call does something.
    ///        Call from the io thread before the first folder is loaded.
    void Load(const AppSettings& appSettings);

    /// @brief Gets the file type for an extension (without dot) that has an emulator. Io thread.
    /// @return The file type, or nullptr when there is no emulator for the extension.
    const FileType* GetFileType(const char* extension) const;

    /// @brief Finds a console of systems.json by folder name (case insensitive).
    const DsRunnerConsoles::Info* FindCustomByFolderName(const char* name) const;

    /// @brief Finds a console of systems.json by file extension (without dot, case insensitive).
    const DsRunnerConsoles::Info* FindCustomByExtension(const char* extension) const;

private:
    struct CustomConsole
    {
        DsRunnerConsoles::Info info;
        char folderName[64];
        char label[20];
        char fullName[64];
        char extensions[MaxExtensions][8];
        u32 extensionCount;
        std::unique_ptr<char[]> emulatorPath;
    };

    bool _loadStarted = false;
    /// Set when everything is filled in, the main thread only reads after that.
    volatile bool _loaded = false;
    std::unique_ptr<CustomConsole[]> _customConsoles;
    u32 _customConsoleCount = 0;
    std::unique_ptr<DsRunnerEmulatorFileType[]> _fileTypes;
    u32 _fileTypeCount = 0;
    DsRunnerEmulatorFinder _emulatorFinder;

    void LoadSystemsJson();
    void CreateFileTypes(const AppSettings& appSettings);
    void AddFileType(const AppSettings& appSettings, u32 capacity, const char* extension, const char* emulatorPath);
};

extern DsRunnerSystems gDsRunnerSystems;

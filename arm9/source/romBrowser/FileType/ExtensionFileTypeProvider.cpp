#include "common.h"
#include <string.h>
#include "UnknownFileType.h"
#include "Nds/NdsFileType.h"
#include "Gba/GbaFileType.h"
#include "ExtensionFileTypeProvider.h"
#include "romBrowser/Theme/dsrunner/DsRunnerSystems.h" // DsRunner

static bool isNdsExtension(const char* extension)
{
    return !strcasecmp(extension, "nds")
        || !strcasecmp(extension, "srl")
        || !strcasecmp(extension, "dsi");
}

static bool isGbaExtension(const char* extension)
{
    return !strcasecmp(extension, "gba")
        || !strcasecmp(extension, "agb");
}

ExtensionFileTypeProvider::ExtensionFileTypeProvider(const AppSettings& appSettings)
    : _appSettings(appSettings)
{
    _customFileTypes = std::make_unique_for_overwrite<CustomFileType[]>(appSettings.numberOfFileAssociations);
    for (u32 i = 0; i < appSettings.numberOfFileAssociations; i++)
    {
        const FileType* baseFileType = nullptr;
        if (isGbaExtension(appSettings.fileAssociations[i].extension))
        {
            baseFileType = &GbaFileType::sInstance;
        }
        _customFileTypes[i] = CustomFileType(&appSettings.fileAssociations[i], baseFileType);
    }
}

const FileType* ExtensionFileTypeProvider::GetFileType(const TCHAR* path) const
{
    const char* extension = strrchr(path, '.');
    if (extension)
    {
        extension++; // skip over dot

        if (isNdsExtension(extension))
        {
            return &NdsFileType::sInstance;
        }

        for (u32 i = 0; i < _appSettings.numberOfFileAssociations; i++)
        {
            if (!strcasecmp(extension, _customFileTypes[i].GetShortName()))
            {
                return &_customFileTypes[i];
            }
        }

        // DsRunner: emulators from /_dsrunner/systems.json or found on the sd card (after settings.json)
        if (const FileType* emulatorFileType = gDsRunnerSystems.GetFileType(extension))
        {
            return emulatorFileType;
        }
    }

    return &UnknownFileType::sInstance;
}
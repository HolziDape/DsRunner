#include "common.h"
#include <string.h>
#include <memory>
#include "JsonAppSettingsService.h"

// Places GBARunner2 is looked for when no .gba file association is configured.
static const char* const sGbaRunnerPaths[] =
{
    "/_dsrunner/GBARunner2.nds",
    "/GBARunner2_arm7dldi_ds.nds",
    "/GBARunner2.nds",
    "/_nds/GBARunner2_arm7dldi_ds.nds",
    "/_nds/GBARunner2.nds",
};

static bool hasFileAssociation(const AppSettings& appSettings, const char* extension)
{
    for (u32 i = 0; i < appSettings.numberOfFileAssociations; i++)
    {
        if (!strcasecmp(appSettings.fileAssociations[i].extension.GetString(), extension))
        {
            return true;
        }
    }
    return false;
}

static void addFileAssociation(AppSettings& appSettings, const char* extension, const char* applicationPath)
{
    u32 count = appSettings.numberOfFileAssociations;
    auto newAssociations = std::make_unique_for_overwrite<FileAssociation[]>(count + 1);
    for (u32 i = 0; i < count; i++)
    {
        newAssociations[i] = appSettings.fileAssociations[i];
    }
    newAssociations[count] = FileAssociation(extension, applicationPath);
    appSettings.fileAssociations = std::move(newAssociations);
    appSettings.numberOfFileAssociations = count + 1;
}

// Without any configuration .gba files are simply not shown. When GBARunner2 is on the
// sd card, use it so GBA games work out of the box. The user can still override this with
// a "gba" entry in the fileAssociations of settings.json.
static void addDefaultFileAssociations(AppSettings& appSettings)
{
    if (hasFileAssociation(appSettings, "gba"))
    {
        return;
    }
    for (const char* path : sGbaRunnerPaths)
    {
        FILINFO fileInfo;
        if (f_stat(path, &fileInfo) == FR_OK && !(fileInfo.fattrib & AM_DIR))
        {
            addFileAssociation(appSettings, "gba", path);
            return;
        }
    }
}

JsonAppSettingsService::JsonAppSettingsService(const char* filePath)
    : _filePath(filePath)
{
    if (!_serializer.Deserialize(&_appSettings, _filePath))
        Save();
    addDefaultFileAssociations(_appSettings);
}

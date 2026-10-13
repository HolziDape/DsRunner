#include "common.h"
#include "JsonAppSettingsService.h"

JsonAppSettingsService::JsonAppSettingsService(const char* filePath)
    : _filePath(filePath)
{
    if (!_serializer.Deserialize(&_appSettings, _filePath))
        Save();
    // DsRunner: emulators like GBARunner2 are found by DsRunnerSystems (romBrowser/Theme/dsrunner),
    // without writing them to settings.json
}

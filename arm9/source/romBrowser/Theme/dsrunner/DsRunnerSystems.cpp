#include "common.h"
#include <string.h>
#include <memory>
#include "json/ArduinoJson.h"
#include "fat/File.h"
#include "services/settings/AppSettings.h"
#include "romBrowser/FileType/Gba/GbaFileType.h"
#include "DsRunnerSystems.h"

// systems.json is small, the document is freed right after reading
#define JSON_RESERVED_SIZE  6144

DsRunnerSystems gDsRunnerSystems;

using DsRunnerConsoles::Color;

static bool isNdsExtension(const char* extension)
{
    return !strcasecmp(extension, "nds") || !strcasecmp(extension, "srl") || !strcasecmp(extension, "dsi");
}

static bool isGbaExtension(const char* extension)
{
    return !strcasecmp(extension, "gba") || !strcasecmp(extension, "agb");
}

static bool hasFileAssociation(const AppSettings& appSettings, const char* extension)
{
    for (u32 i = 0; i < appSettings.numberOfFileAssociations; i++)
    {
        if (!strcasecmp(appSettings.fileAssociations[i].extension.GetString(), extension))
            return true;
    }
    return false;
}

static int hexDigit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

// "#RRGGBB", "RRGGBB" or [r, g, b] (0 to 255)
static bool tryParseColor(JsonVariantConst json, Color& color)
{
    if (json.is<const char*>())
    {
        const char* text = json.as<const char*>();
        if (text[0] == '#')
            text++;
        if (strlen(text) != 6)
            return false;
        u8 values[3];
        for (int i = 0; i < 3; i++)
        {
            int high = hexDigit(text[i * 2]);
            int low = hexDigit(text[i * 2 + 1]);
            if (high < 0 || low < 0)
                return false;
            values[i] = high * 16 + low;
        }
        color = Color(values[0], values[1], values[2]);
        return true;
    }
    if (json.is<JsonArrayConst>())
    {
        JsonArrayConst array = json.as<JsonArrayConst>();
        if (array.size() != 3)
            return false;
        int values[3];
        for (int i = 0; i < 3; i++)
        {
            if (!array[i].is<int>())
                return false;
            values[i] = array[i].as<int>();
            if (values[i] < 0 || values[i] > 255)
                return false;
        }
        color = Color(values[0], values[1], values[2]);
        return true;
    }
    return false;
}

// the gradient on the top screen is a bit darker than the tile, like in the built-in list
static Color heroFromTile(const Color& tile)
{
    return Color(tile.r * 13 / 16, tile.g * 13 / 16, tile.b * 13 / 16);
}

static void copyText(char* destination, u32 size, const char* source)
{
    strlcpy(destination, source ? source : "", size);
}

void DsRunnerSystems::Load(const AppSettings& appSettings)
{
    if (_loadStarted)
        return;
    _loadStarted = true;

    LoadSystemsJson();
    _emulatorFinder.Scan();
    CreateFileTypes(appSettings);

    // everything is in place before the main thread may look at it
    asm volatile("" ::: "memory");
    _loaded = true;
}

void DsRunnerSystems::LoadSystemsJson()
{
    auto file = std::make_unique<File>();
    if (file->Open(FilePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return; // optional

    u32 fileSize = file->GetSize();
    if (fileSize == 0 || fileSize > MaxFileSize)
    {
        LOG_WARNING("DsRunner: %s ignored (size %d, max %d)\n", FilePath, fileSize, MaxFileSize);
        return;
    }

    auto fileData = std::make_unique<char[]>(fileSize + 1);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
    {
        LOG_WARNING("DsRunner: %s could not be read\n", FilePath);
        return;
    }
    fileData[fileSize] = 0;
    file.reset();

    auto json = std::make_unique<DynamicJsonDocument>(JSON_RESERVED_SIZE);
    auto error = deserializeJson(*json, fileData.get(), fileSize);
    if (error != DeserializationError::Ok)
    {
        LOG_WARNING("DsRunner: %s ignored, invalid json: %s\n", FilePath, error.c_str());
        return;
    }

    JsonObjectConst systems = (*json)["systems"].as<JsonObjectConst>();
    if (systems.isNull())
    {
        LOG_WARNING("DsRunner: %s ignored, \"systems\" is missing or no object\n", FilePath);
        return;
    }

    u32 capacity = std::min<u32>(systems.size(), MaxCustomConsoles);
    if (systems.size() > MaxCustomConsoles)
        LOG_WARNING("DsRunner: %s has more than %d systems, the rest is ignored\n", FilePath, MaxCustomConsoles);
    if (capacity == 0)
        return;

    _customConsoles = std::make_unique<CustomConsole[]>(capacity);
    _customConsoleCount = 0;
    for (JsonPairConst pair : systems)
    {
        if (_customConsoleCount >= capacity)
            break;

        const char* folderName = pair.key().c_str();
        JsonObjectConst entry = pair.value().as<JsonObjectConst>();
        if (!folderName || folderName[0] == 0 || strlen(folderName) >= sizeof(CustomConsole::folderName) ||
            entry.isNull())
        {
            LOG_WARNING("DsRunner: systems.json: entry \"%s\" ignored\n", folderName ? folderName : "");
            continue;
        }

        auto& console = _customConsoles[_customConsoleCount];
        const auto* builtIn = DsRunnerConsoles::FindBuiltInByFolderName(folderName);

        copyText(console.folderName, sizeof(console.folderName), folderName);
        // defaults: the built-in console, or a neutral tile with the folder name like an unknown folder
        copyText(console.label, sizeof(console.label), entry["label"] | (builtIn ? builtIn->label : folderName));
        copyText(console.fullName, sizeof(console.fullName), entry["name"] | (builtIn ? builtIn->fullName : folderName));

        Color tile = builtIn ? builtIn->tile : DsRunnerConsoles::kUnknownTile;
        Color hero = builtIn ? builtIn->hero : DsRunnerConsoles::kUnknownHero;
        if (!entry["color"].isNull())
        {
            if (tryParseColor(entry["color"], tile))
                hero = heroFromTile(tile);
            else
                LOG_WARNING("DsRunner: systems.json: invalid color of \"%s\"\n", folderName);
        }
        if (!entry["heroColor"].isNull() && !tryParseColor(entry["heroColor"], hero))
            LOG_WARNING("DsRunner: systems.json: invalid heroColor of \"%s\"\n", folderName);

        console.extensionCount = 0;
        JsonArrayConst extensions = entry["extensions"].as<JsonArrayConst>();
        if (!extensions.isNull())
        {
            for (JsonVariantConst value : extensions)
            {
                const char* extension = value.as<const char*>();
                if (!extension)
                    continue;
                if (extension[0] == '.')
                    extension++;
                if (extension[0] == 0 || strlen(extension) >= sizeof(console.extensions[0]))
                {
                    LOG_WARNING("DsRunner: systems.json: extension \"%s\" ignored\n", extension);
                    continue;
                }
                if (console.extensionCount >= MaxExtensions)
                {
                    LOG_WARNING("DsRunner: systems.json: more than %d extensions for \"%s\"\n", MaxExtensions, folderName);
                    break;
                }
                strcpy(console.extensions[console.extensionCount++], extension);
            }
        }
        else if (builtIn)
        {
            // an override of a built-in console keeps its extensions
            for (const auto& builtInExtension : DsRunnerConsoles::kExtensions)
            {
                if (console.extensionCount < MaxExtensions &&
                    !strcasecmp(builtInExtension.folderName, folderName))
                {
                    strcpy(console.extensions[console.extensionCount++], builtInExtension.extension);
                }
            }
        }

        const char* emulator = entry["emulator"].as<const char*>();
        if (emulator && emulator[0] != 0)
        {
            auto fileInfo = std::make_unique<FILINFO>();
            u32 length = strlen(emulator);
            if (length >= sizeof(pload_params_t::romPath))
            {
                LOG_WARNING("DsRunner: systems.json: emulator path of \"%s\" too long\n", folderName);
            }
            else if (f_stat(emulator, fileInfo.get()) != FR_OK || (fileInfo->fattrib & AM_DIR))
            {
                // the auto detected emulator (if any) stays in charge
                LOG_WARNING("DsRunner: systems.json: emulator %s of \"%s\" not found\n", emulator, folderName);
            }
            else
            {
                console.emulatorPath = std::make_unique<char[]>(length + 1);
                memcpy(console.emulatorPath.get(), emulator, length + 1);
            }
        }

        console.info = DsRunnerConsoles::Info
        {
            console.folderName, console.label, console.fullName, tile, hero
        };
        _customConsoleCount++;
    }
    LOG_INFO("DsRunner: %d systems from %s\n", _customConsoleCount, FilePath);
}

void DsRunnerSystems::CreateFileTypes(const AppSettings& appSettings)
{
    u32 capacity = sizeof(DsRunnerConsoles::kExtensions) / sizeof(DsRunnerConsoles::kExtensions[0]);
    for (u32 i = 0; i < _customConsoleCount; i++)
        capacity += _customConsoles[i].extensionCount;
    _fileTypes = std::make_unique<DsRunnerEmulatorFileType[]>(capacity);
    _fileTypeCount = 0;

    // 1. emulators of systems.json
    for (u32 i = 0; i < _customConsoleCount; i++)
    {
        const auto& console = _customConsoles[i];
        if (!console.emulatorPath)
            continue;
        for (u32 j = 0; j < console.extensionCount; j++)
            AddFileType(appSettings, capacity, console.extensions[j], console.emulatorPath.get());
    }

    // 2. emulators found on the sd card, for the extensions of the built-in consoles
    for (const auto& entry : DsRunnerConsoles::kExtensions)
        AddFileType(appSettings, capacity, entry.extension, _emulatorFinder.FindForConsole(entry.folderName));

    // 3. extensions of systems.json consoles without an emulator of their own, but of a built-in console
    //    (e.g. "SEGA": ["sms", "gg"] uses S8DS) are covered by step 2 already.
}

void DsRunnerSystems::AddFileType(const AppSettings& appSettings, u32 capacity,
    const char* extension, const char* emulatorPath)
{
    if (!emulatorPath || _fileTypeCount >= capacity || isNdsExtension(extension))
        return;
    // settings.json comes first (ExtensionFileTypeProvider), so these would never be used
    if (hasFileAssociation(appSettings, extension))
        return;
    for (u32 i = 0; i < _fileTypeCount; i++)
    {
        if (!strcasecmp(_fileTypes[i].GetExtension(), extension))
            return; // the first one wins
    }
    const FileType* baseFileType = isGbaExtension(extension) ? &GbaFileType::sInstance : nullptr;
    _fileTypes[_fileTypeCount++] = DsRunnerEmulatorFileType(extension, emulatorPath, baseFileType);
    LOG_DEBUG("DsRunner: .%s -> %s\n", extension, emulatorPath);
}

const FileType* DsRunnerSystems::GetFileType(const char* extension) const
{
    if (!_loaded || !extension)
        return nullptr;
    for (u32 i = 0; i < _fileTypeCount; i++)
    {
        if (!strcasecmp(_fileTypes[i].GetExtension(), extension))
            return &_fileTypes[i];
    }
    return nullptr;
}

const DsRunnerConsoles::Info* DsRunnerSystems::FindCustomByFolderName(const char* name) const
{
    if (!_loaded || !name)
        return nullptr;
    for (u32 i = 0; i < _customConsoleCount; i++)
    {
        if (!strcasecmp(_customConsoles[i].folderName, name))
            return &_customConsoles[i].info;
    }
    return nullptr;
}

const DsRunnerConsoles::Info* DsRunnerSystems::FindCustomByExtension(const char* extension) const
{
    if (!_loaded || !extension)
        return nullptr;
    for (u32 i = 0; i < _customConsoleCount; i++)
    {
        const auto& console = _customConsoles[i];
        for (u32 j = 0; j < console.extensionCount; j++)
        {
            if (!strcasecmp(console.extensions[j], extension))
                return &console.info;
        }
    }
    return nullptr;
}

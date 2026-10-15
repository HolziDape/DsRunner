#include "common.h"
#include <string.h>
#include <memory>
#include "core/mini-printf.h"
#include "core/math/RgbMixer.h"
#include "fat/File.h"
#include "json/ArduinoJson.h"
#include "DsRunnerColorScheme.h"

#pragma GCC optimize("Os")

// theme.json is small (Pico's ThemeInfoFactory reads it with 2048), "colors" adds up to ~30 entries
#define JSON_RESERVED_SIZE  4096
#define MAX_FILE_SIZE       8192

namespace DsRunnerColors
{
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

    static bool isByte(JsonVariantConst json)
    {
        return json.is<int>() && json.as<int>() >= 0 && json.as<int>() <= 255;
    }

    // "#RRGGBB", "RRGGBB", [r, g, b] or { "r": .., "g": .., "b": .. } (0 to 255, like primaryColor)
    static bool tryParseColor(JsonVariantConst json, Rgb888& color)
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
            color = Rgb888(values[0], values[1], values[2]);
            return true;
        }
        if (json.is<JsonArrayConst>())
        {
            JsonArrayConst array = json.as<JsonArrayConst>();
            if (array.size() != 3 || !isByte(array[0]) || !isByte(array[1]) || !isByte(array[2]))
                return false;
            color = Rgb888(array[0].as<int>(), array[1].as<int>(), array[2].as<int>());
            return true;
        }
        if (json.is<JsonObjectConst>())
        {
            JsonObjectConst object = json.as<JsonObjectConst>();
            if (!isByte(object["r"]) || !isByte(object["g"]) || !isByte(object["b"]))
                return false;
            color = Rgb888(object["r"].as<int>(), object["g"].as<int>(), object["b"].as<int>());
            return true;
        }
        return false;
    }

    // reads colors[key] into color, else color = fallback
    static void read(JsonObjectConst colors, const char* key, Rgb888& color, const Rgb888& fallback)
    {
        JsonVariantConst value = colors[key];
        if (value.isNull())
        {
            color = fallback;
            return;
        }
        if (!tryParseColor(value, color))
        {
            LOG_WARNING("DsRunner: theme color \"%s\" is invalid, use \"#RRGGBB\"\n", key);
            color = fallback;
        }
    }

    static void resetToDefaults()
    {
        BgTop = Defaults::BgTop;
        BgBottom = Defaults::BgBottom;
        Tile = Defaults::Tile;
        TileDim = Defaults::TileDim;
        TileTop = Defaults::TileTop;
        TileSelectedTop = Defaults::TileSelectedTop;
        TileSelectedBottom = Defaults::TileSelectedBottom;
        TileShade = Defaults::BgTop;
        Accent = Defaults::Accent;
        AccentDim = Defaults::AccentDim;
        OnAccent = Defaults::BgTop;
        Text = Defaults::Text;
        TextDim = Defaults::TextDim;
        TextStrong = Defaults::TextStrong;
        Pill = Defaults::Pill;
        BatteryGood = Defaults::BatteryGood;
        BatteryLow = Defaults::BatteryLow;
        Favorite = Defaults::Favorite;
        ArtShade = Defaults::BgTop;
        KeyTop = Defaults::KeyTop;
        KeyBottom = Defaults::KeyBottom;
        KeySpecialTop = Defaults::KeySpecialTop;
        KeySpecialBottom = Defaults::KeySpecialBottom;
        KeySelectedTop = Defaults::KeySelectedTop;
        KeySelectedBottom = Defaults::KeySelectedBottom;
        FieldTop = Defaults::FieldTop;
        FieldBottom = Defaults::FieldBottom;
        FieldStatus = Defaults::FieldStatus;
    }

    // A theme with "colors" only needs the main colors: the others are mixed from them
    // (for DsRunner Dark's values the results are close to Defaults).
    static void readColors(JsonObjectConst colors)
    {
        read(colors, "background", BgTop, Defaults::BgTop);
        read(colors, "text", Text, Defaults::Text);
        read(colors, "tile", Tile, Defaults::Tile);
        read(colors, "backgroundBottom", BgBottom, RgbMixer::Lerp(BgTop, Tile, 1, 2));
        read(colors, "accent", Accent, Defaults::Accent);
        read(colors, "textDim", TextDim, RgbMixer::Lerp(Text, BgTop, 3, 8));
        read(colors, "textStrong", TextStrong, Text);
        read(colors, "pill", Pill, RgbMixer::Lerp(Tile, Text, 1, 20));
        read(colors, "favorite", Favorite, Defaults::Favorite);
        read(colors, "onAccent", OnAccent, BgTop);
        read(colors, "accentDim", AccentDim, RgbMixer::Lerp(Accent, BgTop, 1, 5));
        read(colors, "tileDim", TileDim, RgbMixer::Lerp(Tile, BgTop, 3, 8));
        read(colors, "tileTop", TileTop, RgbMixer::Lerp(Tile, BgTop, 1, 8));
        read(colors, "tileSelected", TileSelectedTop, RgbMixer::Lerp(Tile, Text, 1, 8));
        read(colors, "tileSelectedBottom", TileSelectedBottom, RgbMixer::Lerp(Tile, Text, 1, 32));
        read(colors, "tileShade", TileShade, BgTop);
        read(colors, "artShade", ArtShade, BgTop);
        read(colors, "batteryGood", BatteryGood, Defaults::BatteryGood);
        read(colors, "batteryLow", BatteryLow, Defaults::BatteryLow);

        // the search keyboard follows the tiles
        KeyTop = Tile;
        KeyBottom = RgbMixer::Lerp(Tile, BgTop, 1, 3);
        KeySpecialTop = RgbMixer::Lerp(Pill, Text, 1, 24);
        KeySpecialBottom = Pill;
        KeySelectedTop = RgbMixer::Lerp(TileSelectedTop, Text, 1, 16);
        KeySelectedBottom = TileSelectedBottom;
        FieldTop = RgbMixer::Lerp(BgTop, BgBottom, 1, 4);
        FieldBottom = RgbMixer::Lerp(BgTop, BgBottom, 3, 4);
        FieldStatus = FieldTop;
    }

    void LoadFromThemeFolder(const char* themeFolderName)
    {
        // the theme may have been switched since the last load: start from DsRunner Dark
        resetToDefaults();

        char path[128];
        mini_snprintf(path, sizeof(path), "/_pico/themes/%s/theme.json", themeFolderName);
        auto file = std::make_unique<File>();
        if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
            return;
        u32 fileSize = file->GetSize();
        if (fileSize == 0 || fileSize > MAX_FILE_SIZE)
            return;
        auto fileData = std::make_unique<char[]>(fileSize + 1);
        u32 bytesRead = 0;
        if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
            return;
        fileData[fileSize] = 0;
        file.reset();

        auto json = std::make_unique<DynamicJsonDocument>(JSON_RESERVED_SIZE);
        if (deserializeJson(*json, fileData.get(), fileSize) != DeserializationError::Ok)
        {
            LOG_WARNING("DsRunner: %s is no valid json, default colors\n", path);
            return;
        }
        JsonObjectConst colors = (*json)["colors"].as<JsonObjectConst>();
        if (colors.isNull())
            return; // DsRunner Dark
        readColors(colors);
        LOG_INFO("DsRunner: theme colors from %s\n", path);
    }
}

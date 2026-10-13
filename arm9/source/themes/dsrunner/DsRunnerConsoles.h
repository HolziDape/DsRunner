#pragma once
#include <string.h>
#include "core/math/Rgb.h"

/// @brief Known game consoles with the colors and labels the DsRunner theme uses for them.
///        Folders are recognized by name (case insensitive), files by extension.
///        The built-in list below can be extended and overridden with /_dsrunner/systems.json
///        (see DsRunnerSystems), the Find functions look there first.
namespace DsRunnerConsoles
{
    using Color = Rgb<8, 8, 8>;

    struct Info
    {
        /// Folder name this entry matches, e.g. "NDS".
        const char* folderName;
        /// Short label drawn on the tile (max 4 characters).
        const char* label;
        /// Name shown as title on the top screen.
        const char* fullName;
        /// Color of the console tile on the bottom screen.
        Color tile;
        /// Color at the top of the gradient on the top screen.
        Color hero;
    };

    // Keep in sync with tools/dsrunner-art/dsrunnerart/consoles.py (CONSOLES).
    inline constexpr Info kConsoles[] =
    {
        { "NDS",       "NDS",  "Nintendo DS",  Color( 58,  92, 190), Color( 44,  74, 168) },
        { "DS",        "NDS",  "Nintendo DS",  Color( 58,  92, 190), Color( 44,  74, 168) },
        { "GBA",       "GBA",  "Game Boy Advance",  Color(112,  64, 184), Color( 92,  52, 156) },
        { "GBC",       "GBC",  "Game Boy Color",  Color(198,  92,  72), Color(168,  70,  54) },
        { "GB",        "GB",   "Game Boy",  Color( 96, 144,  64), Color( 74, 116,  48) },
        { "NES",       "NES",  "Nintendo Entertainment System",  Color(190,  48,  48), Color(160,  36,  36) },
        { "FC",        "NES",  "Nintendo Entertainment System",  Color(190,  48,  48), Color(160,  36,  36) },
        { "SNES",      "SNES", "Super Nintendo", Color(112,  92, 172), Color( 92,  74, 148) },
        { "SFC",       "SNES", "Super Nintendo", Color(112,  92, 172), Color( 92,  74, 148) },
        { "SMS",       "SMS",  "Sega Master System",  Color( 44, 112, 192), Color( 32,  92, 166) },
        { "GG",        "GG",   "Sega Game Gear",  Color( 92,  92, 108), Color( 72,  72,  88) },
        { "GEN",       "MD",   "Sega Mega Drive",  Color( 76,  76,  90), Color( 58,  58,  72) },
        { "MD",        "MD",   "Sega Mega Drive",  Color( 76,  76,  90), Color( 58,  58,  72) },
        { "MEGADRIVE", "MD",   "Sega Mega Drive",  Color( 76,  76,  90), Color( 58,  58,  72) },
        { "PCE",       "PCE",  "PC Engine",  Color(150, 150, 166), Color(116, 116, 134) },
        { "APPS",      "APP",  "Apps und Homebrew",  Color( 32, 160, 150), Color( 24, 128, 120) },
        { "HOMEBREW",  "APP",  "Apps und Homebrew",  Color( 32, 160, 150), Color( 24, 128, 120) },
        // the virtual favorites folder (DsRunnerFavoritesFolder::EntryName, "★ Favoriten"), label "★"
        { "\xE2\x98\x85 Favoriten", "\xE2\x98\x85", "Favoriten", Color(196, 146, 40), Color(150, 108, 24) },
    };

    struct ExtensionInfo
    {
        /// File extension without the dot, lower case.
        const char* extension;
        /// Folder name of the console in kConsoles.
        const char* folderName;
    };

    /// File extensions of the built-in consoles. Files of these types are shown when Pico can start them
    /// (nds) or an emulator for them is known (file association, systems.json or found on the sd card,
    /// see DsRunnerSystems). Keep in sync with SYSTEMS in tools/dsrunner-art/dsrunnerart/consoles.py.
    inline constexpr ExtensionInfo kExtensions[] =
    {
        { "nds", "NDS" }, { "srl", "NDS" }, { "dsi", "NDS" },
        { "gba", "GBA" }, { "agb", "GBA" },
        { "gbc", "GBC" },
        { "gb",  "GB" }, { "sgb", "GB" },
        { "nes", "NES" }, { "fds", "NES" },
        { "sfc", "SNES" }, { "smc", "SNES" },
        { "sms", "SMS" },
        { "gg",  "GG" },
        { "gen", "MD" }, { "md", "MD" }, { "smd", "MD" },
        { "pce", "PCE" }, { "sgx", "PCE" },
    };

    /// Tile / hero color of folders and files that are not a known console.
    inline constexpr Color kUnknownTile = Color(64, 74, 100);
    inline constexpr Color kUnknownHero = Color(40, 48, 70);

    /// @brief Finds a console of the built-in list by folder name (ignores systems.json).
    inline const Info* FindBuiltInByFolderName(const char* name)
    {
        if (!name)
            return nullptr;
        for (const auto& console : kConsoles)
        {
            if (!strcasecmp(console.folderName, name))
                return &console;
        }
        return nullptr;
    }

    /// @brief Gets the built-in console folder of a file extension (without dot), or nullptr.
    inline const char* FindBuiltInFolderByExtension(const char* extension)
    {
        if (!extension)
            return nullptr;
        for (const auto& entry : kExtensions)
        {
            if (!strcasecmp(entry.extension, extension))
                return entry.folderName;
        }
        return nullptr;
    }

    /// @brief Finds the console that belongs to a folder name: systems.json first, then the built-in list.
    /// @return The console info, or nullptr when the name is not a known console.
    const Info* FindByFolderName(const char* name);

    /// @brief Finds the console that belongs to the extension of a file name: systems.json first,
    ///        then the built-in list.
    /// @return The console info, or nullptr when the extension is unknown.
    const Info* FindByFileName(const char* fileName);

    /// @brief Gets the hero (top screen gradient) color for an item name that may be a folder or a file.
    inline Color GetHeroColor(const char* name)
    {
        const Info* info = FindByFolderName(name);
        if (!info)
            info = FindByFileName(name);
        return info ? info->hero : kUnknownHero;
    }
}

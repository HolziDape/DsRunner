#pragma once
#include "common.h"

/// @brief Emulator homebrew DsRunner finds on the sd card by itself, so roms of other consoles
///        work without a file association in settings.json (like GBARunner2 for GBA since Phase 2).
///        The rom is passed as argv[1], the same way as for Pico's file associations.
namespace DsRunnerEmulators
{
    struct Emulator
    {
        /// Name for the log.
        const char* name;
        /// Built-in console folders (DsRunnerConsoles::kExtensions) the emulator plays, nullptr terminated.
        const char* consoles[3];
        /// File names to look for, in this order (case insensitive), nullptr terminated.
        const char* fileNames[3];
    };

    /// Emulators in order of preference: for a console the first one found is used.
    /// Keep in sync with docs/CONFIG.md ("Emulatoren").
    inline constexpr Emulator kEmulators[] =
    {
        { "GBARunner2", { "GBA" }, { "GBARunner2_arm7dldi_ds.nds", "GBARunner2.nds", "GBARunner2_arm9dldi_ds.nds" } },
        { "nesDS",      { "NES" }, { "nesDS.nds" } },
        { "GameYob",    { "GB", "GBC" }, { "GameYob.nds" } },
        { "lameboy",    { "GB", "GBC" }, { "lameboy.nds" } },
        { "S8DS",       { "SMS", "GG" }, { "S8DS.nds" } },
        { "jEnesisDS",  { "MD" }, { "jEnesisDS.nds" } },
        { "SNEmulDS",   { "SNES" }, { "SNEmulDS-legacy.nds", "SNEmulDS.nds" } },
        { "NitroGrafx", { "PCE" }, { "NitroGrafx.nds" } },
    };

    inline constexpr u32 kEmulatorCount = sizeof(kEmulators) / sizeof(kEmulators[0]);

    /// Folders that are searched, in this order. Each folder is read once at startup.
    inline constexpr const char* kSearchFolders[] =
    {
        "/_dsrunner/emulators",
        "/_dsrunner",
        "/",
        "/_nds",
        "/_nds/TWiLightMenu/emulators",
        "/Emulators",
    };

    inline constexpr u32 kSearchFolderCount = sizeof(kSearchFolders) / sizeof(kSearchFolders[0]);

    /// Longest path of a found emulator ("/_nds/TWiLightMenu/emulators/" + file name) plus terminator.
    inline constexpr u32 kMaxPathLength = 64;
}

/// @brief Looks for the emulators of DsRunnerEmulators::kEmulators on the sd card.
class DsRunnerEmulatorFinder
{
public:
    /// @brief Reads each search folder once and remembers the emulators found. Call from the io thread.
    void Scan();

    /// @brief Gets the emulator for a built-in console (canonical folder name like "GB", "MD").
    /// @return The path of the first emulator found for the console, or nullptr.
    const char* FindForConsole(const char* consoleFolder) const;

private:
    char _paths[DsRunnerEmulators::kEmulatorCount][DsRunnerEmulators::kMaxPathLength] = { };
};

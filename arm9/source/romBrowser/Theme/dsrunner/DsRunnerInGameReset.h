#pragma once
#include "common.h"

/// @brief DsRunner: "zurück zum Launcher" from retail games (Phase 8, see docs/LOADER.md).
///        Holding L+R+Down+B for two seconds in a game reboots into the DsRunner build of
///        Pico Loader (loader/), which then boots the launcher path it saved on the card.
namespace DsRunnerInGameReset
{
    /// Argument Pico Loader appends after argv[0] when it boots the launcher after an in-game reset.
    inline constexpr const char* kLauncherArgument = "igr";

    /// When this file exists, the DsRunner Pico Loader leaves out the in-game reset patch.
    /// Must match IN_GAME_RESET_DISABLE_PATH in loader/patches.
    inline constexpr const char* kDisableFilePath = "/_pico/igr.off";

    /// Launchers used when argv[0] is missing, in this order. The first is the file the
    /// M3 DS Real autoboot chain starts (tools/make-sd-package.sh), the others are copies
    /// to start by hand. The loader has a similar list (sFallbackLauncherPaths in
    /// loader/patches) for when its saved launcher path is missing.
    inline constexpr const char* kFallbackLauncherPaths[] =
    {
        "/_nds/TWiLightMenu/main.srldr",
        "/BOOT.NDS",
        "/DsRunner.nds",
    };

    /// @brief Picks the path Pico Loader returns to: argv[0] when it is set and exists on the
    ///        card, otherwise the first existing kFallbackLauncherPaths entry. Logs the choice.
    ///        Call after the card is mounted.
    /// @return The launcher path, or nullptr when none was found.
    const char* ChooseLauncherPath(int argc, char* argv[]);

    /// @brief Returns whether DsRunner was booted by Pico Loader after an in-game reset.
    bool IsInGameResetBoot(int argc, char* argv[]);

    /// @brief Creates or deletes kDisableFilePath so that Pico Loader follows the setting
    ///        "inGameReset" of settings.json. Only writes to the card when the state changes.
    void ApplySetting(bool inGameResetEnabled);
}

#pragma once
#include "common.h"

/// @brief Texts of the DsRunner UI in German and English (UTF-8). The language comes from
///        settings.json "uiLanguage" ("de" or "en", see DsRunnerText::IsEnglishSetting) and can be
///        switched in the display settings while DsRunner runs.
///        Internal names like DsRunnerFavoritesFolder::EntryName stay German, Localize() translates
///        them where they are shown.
namespace DsRunnerText
{
    enum class Id
    {
        // display settings
        MenuDisplayTitle,
        MenuLayout,
        MenuSort,
        MenuClock,
        MenuGameTitle,
        MenuInGameReset,
        MenuKeysCustom,
        // app bar
        AppBarConsoles,
        AppBarSdCard,
        // virtual folders of the console level
        Favorites,
        Search,
        // search results
        SearchTitle,          // "Suche: <query> · <filter> (<count>)"
        SearchTitlePartial,   // "Suche: <query> · <filter> (<shown> von <count>)"
        SearchFilterAll,
        SearchGroupOther,
        // info card on the top screen
        SearchTileHint,
        ContinuePrefix,       // "START: Weiterspielen (" + game name + ")"
        SearchGamesOne,
        SearchGamesMany,      // %d
        NoGames,
        GamesOne,
        GamesMany,            // %d
        StatsNeverPlayed,
        StatsPlayedBefore,    // only in recent.txt, from before the starts were counted
        StatsPlayCount,       // %d
        StatsLastPlayed,      // " · zuletzt " + StatsToday, StatsYesterday, StatsDaysAgo or a date
        StatsToday,
        StatsYesterday,
        StatsDaysAgo,         // %d
        // search keyboard
        KeySpace,
        KeySearch,
        KeyboardCancel,
        KeyboardHint,
        KeyboardPlaceholder,
        KeyboardReadingGames,
        KeyboardNoMatches,    // %s
        KeyboardMoreMatches,  // %d %d
        KeyboardMatchesOne,
        KeyboardMatchesMany,  // %d
        // Pico views
        NoPreview,
        NoCheats,
        FavoriteChip,

        Count
    };

    /// @brief Returns true when \p uiLanguage ("uiLanguage" of settings.json) means English:
    ///        "en" or "english", ignoring case. Everything else is German.
    bool IsEnglishSetting(const char* uiLanguage);

    /// @brief Sets the language of the texts. German is the default.
    void SetEnglish(bool english);

    bool IsEnglish();

    /// @brief Gets a text in the current language.
    const char* Get(Id id);

    /// @brief Writes \p one for \p count 1, else \p many with the count ("1 Spiel", "2 Spiele").
    ///        \p many may contain one %d.
    void FormatCount(char* buffer, u32 bufferSize, Id one, Id many, int count);

    /// @brief Translates a German name that is shown as is: the names of the virtual folders
    ///        ("★ Favoriten", "• Suche") and the built-in console names that are German
    ///        ("Favoriten", "Suche", "Apps und Homebrew"). Other text is returned unchanged.
    const char* Localize(const char* text);
}

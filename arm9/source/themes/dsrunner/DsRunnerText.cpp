#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "DsRunnerText.h"

// The fonts (NotoSansJP up to Medium 11) have umlauts, "·", "…", "„“", "★", "•" and the arrows,
// but no "–" or "—".
struct TextEntry
{
    const char* german;
    const char* english;
};

static constexpr TextEntry sTexts[] =
{
    // display settings
    [(int)DsRunnerText::Id::MenuDisplayTitle] = { "Anzeige", "Display" },
    [(int)DsRunnerText::Id::MenuLayout] = { "Ansicht", "Layout" },
    [(int)DsRunnerText::Id::MenuSort] = { "Sortieren", "Sort" },
    [(int)DsRunnerText::Id::MenuClock] = { "Uhr", "Clock" },
    [(int)DsRunnerText::Id::MenuGameTitle] = { "Spiel & Sprache", "Game & language" },
    [(int)DsRunnerText::Id::MenuInGameReset] = { "Zur\xC3\xBC" "ck aus dem Spiel", "Return from game" },
    [(int)DsRunnerText::Id::MenuKeysCustom] = { "Eigene", "Custom" },
    // app bar
    [(int)DsRunnerText::Id::AppBarConsoles] = { "Konsolen", "Consoles" },
    [(int)DsRunnerText::Id::AppBarSdCard] = { "SD-Karte", "SD card" },
    // virtual folders of the console level
    [(int)DsRunnerText::Id::Favorites] = { "Favoriten", "Favorites" },
    [(int)DsRunnerText::Id::Search] = { "Suche", "Search" },
    // search results
    [(int)DsRunnerText::Id::SearchTitle] = { "Suche: %s \xC2\xB7 %s (%d)", "Search: %s \xC2\xB7 %s (%d)" },
    [(int)DsRunnerText::Id::SearchTitlePartial] =
        { "Suche: %s \xC2\xB7 %s (%d von %d)", "Search: %s \xC2\xB7 %s (%d of %d)" },
    [(int)DsRunnerText::Id::SearchFilterAll] = { "Alle", "All" },
    [(int)DsRunnerText::Id::SearchGroupOther] = { "Weitere", "Other" },
    // info card on the top screen
    [(int)DsRunnerText::Id::SearchTileHint] = { "Auch mit SELECT, aus jedem Ordner", "Also with SELECT, from any folder" },
    [(int)DsRunnerText::Id::ContinuePrefix] = { "START: Weiterspielen (", "START: Continue (" },
    [(int)DsRunnerText::Id::SearchGamesOne] = { "1 Spiel durchsuchen", "Search 1 game" },
    [(int)DsRunnerText::Id::SearchGamesMany] = { "%d Spiele durchsuchen", "Search %d games" },
    [(int)DsRunnerText::Id::NoGames] = { "Keine Spiele", "No games" },
    [(int)DsRunnerText::Id::GamesOne] = { "1 Spiel", "1 game" },
    [(int)DsRunnerText::Id::GamesMany] = { "%d Spiele", "%d games" },
    [(int)DsRunnerText::Id::StatsNeverPlayed] = { "Noch nie gespielt", "Never played" },
    [(int)DsRunnerText::Id::StatsPlayedBefore] = { "Schon gespielt", "Played before" },
    [(int)DsRunnerText::Id::StatsPlayCount] = { "%d\xC3\x97 gespielt", "Played %d\xC3\x97" },
    [(int)DsRunnerText::Id::StatsLastPlayed] = { " \xC2\xB7 zuletzt ", " \xC2\xB7 last " },
    [(int)DsRunnerText::Id::StatsToday] = { "heute", "today" },
    [(int)DsRunnerText::Id::StatsYesterday] = { "gestern", "yesterday" },
    [(int)DsRunnerText::Id::StatsDaysAgo] = { "vor %d Tagen", "%d days ago" },
    // search keyboard
    [(int)DsRunnerText::Id::KeySpace] = { "Leer", "Space" },
    [(int)DsRunnerText::Id::KeySearch] = { "Suchen", "Search" },
    [(int)DsRunnerText::Id::KeyboardCancel] = { "Abbrechen", "Cancel" },
    [(int)DsRunnerText::Id::KeyboardHint] =
    {
        "B: L\xC3\xB6schen \xC2\xB7 X: Leerzeichen \xC2\xB7 START: Suchen \xC2\xB7 SELECT: Zur\xC3\xBC" "ck",
        "B: Delete \xC2\xB7 X: Space \xC2\xB7 START: Search \xC2\xB7 SELECT: Back"
    },
    [(int)DsRunnerText::Id::KeyboardPlaceholder] = { "Name des Spiels", "Name of the game" },
    [(int)DsRunnerText::Id::KeyboardReadingGames] = { "Spiele werden gelesen\xE2\x80\xA6", "Reading games\xE2\x80\xA6" },
    [(int)DsRunnerText::Id::KeyboardNoMatches] =
        { "Keine Treffer f\xC3\xBCr \xE2\x80\x9E%s\xE2\x80\x9C", "No matches for \xE2\x80\x9C%s\xE2\x80\x9D" },
    [(int)DsRunnerText::Id::KeyboardMoreMatches] = { "Mehr als %d Treffer (%d):", "More than %d matches (%d):" },
    [(int)DsRunnerText::Id::KeyboardMatchesOne] = { "1 Treffer:", "1 match:" },
    [(int)DsRunnerText::Id::KeyboardMatchesMany] = { "%d Treffer:", "%d matches:" },
    // Pico views
    [(int)DsRunnerText::Id::NoPreview] = { "Keine Vorschau", "No preview" },
    [(int)DsRunnerText::Id::NoCheats] = { "Keine Cheats gefunden.", "No cheats found." },
    [(int)DsRunnerText::Id::FavoriteChip] = { "Favorit", "Favorite" },
};

static_assert(sizeof(sTexts) / sizeof(sTexts[0]) == (int)DsRunnerText::Id::Count, "a text is missing");

// German names that are shown as they are (folder entries, console names) and their English names
static constexpr TextEntry sNames[] =
{
    { "\xE2\x98\x85 Favoriten", "\xE2\x98\x85 Favorites" },
    { "\xE2\x80\xA2 Suche", "\xE2\x80\xA2 Search" },
    { "Favoriten", "Favorites" },
    { "Suche", "Search" },
    { "Apps und Homebrew", "Apps and homebrew" },
};

static bool sEnglish = false;

bool DsRunnerText::IsEnglishSetting(const char* uiLanguage)
{
    return uiLanguage && (!strcasecmp(uiLanguage, "en") || !strcasecmp(uiLanguage, "english"));
}

void DsRunnerText::SetEnglish(bool english)
{
    sEnglish = english;
}

bool DsRunnerText::IsEnglish()
{
    return sEnglish;
}

const char* DsRunnerText::Get(Id id)
{
    if ((u32)id >= (u32)Id::Count)
        return "";
    const auto& entry = sTexts[(int)id];
    return sEnglish ? entry.english : entry.german;
}

void DsRunnerText::FormatCount(char* buffer, u32 bufferSize, Id one, Id many, int count)
{
    if (count == 1)
        strlcpy(buffer, Get(one), bufferSize);
    else
        mini_snprintf(buffer, bufferSize, Get(many), count);
}

const char* DsRunnerText::Localize(const char* text)
{
    if (!sEnglish || !text)
        return text;
    for (const auto& name : sNames)
    {
        if (!strcmp(text, name.german))
            return name.english;
    }
    return text;
}

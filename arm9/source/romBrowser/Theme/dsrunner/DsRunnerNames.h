#pragma once
#include "common.h"

/// @brief The names games are shown with. Only for display: sorting, the search, favorites and the play
///        history keep using the real file name.
namespace DsRunnerNames
{
    /// @brief Builds the name a game file is shown with: the file name without its extension
    ///        ("Zelda.nes" -> "Zelda"). With \p clean also without the region, language and dump tags
    ///        of No-Intro and GoodTools names: "Mario Kart (Europe) (En,Fr,De) [!].gba" -> "Mario Kart".
    ///        Other tags like "(Rev 1)", "(Beta)" or "(Disc 1)" stay.
    /// @param fileName The file name.
    /// @param clean True to remove the tags.
    /// @param result Receives the name, cut off when it doesn't fit.
    /// @param resultSize The size of \p result in bytes.
    void GetDisplayName(const char* fileName, bool clean, char* result, u32 resultSize);

    /// @brief Like GetDisplayName() above, removes the tags when settings.json "cleanNames" is true (default).
    void GetDisplayName(const char* fileName, char* result, u32 resultSize);
}

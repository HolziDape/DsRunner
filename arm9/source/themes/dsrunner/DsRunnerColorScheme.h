#pragma once
#include "themes/material/MaterialColorScheme.h"

/// @brief The colors of the DsRunner theme type. A theme can change them in its theme.json
///        ("colors", see LoadFromThemeFolder and docs/Themes.md). Without "colors" the theme
///        looks like DsRunner Dark, exactly as before colors were configurable.
namespace DsRunnerColors
{
    using Rgb888 = Rgb<8, 8, 8>;

    /// The colors of DsRunner Dark (the dsrunner theme, no "colors" in theme.json).
    namespace Defaults
    {
        inline constexpr Rgb888 BgTop = Rgb888(15, 17, 23);       // #0F1117
        inline constexpr Rgb888 BgBottom = Rgb888(24, 28, 38);    // #181C26
        inline constexpr Rgb888 Tile = Rgb888(35, 40, 56);        // #232838
        inline constexpr Rgb888 TileDim = Rgb888(28, 32, 45);     // between Tile and BgTop
        inline constexpr Rgb888 TileTop = Rgb888(33, 38, 53);     // unselected game tile, top
        inline constexpr Rgb888 TileSelectedTop = Rgb888(58, 66, 90);
        inline constexpr Rgb888 TileSelectedBottom = Rgb888(40, 46, 64);
        inline constexpr Rgb888 Accent = Rgb888(76, 194, 255);    // #4CC2FF
        inline constexpr Rgb888 AccentDim = Rgb888(48, 150, 214); // lower end of accent gradients
        inline constexpr Rgb888 Text = Rgb888(235, 238, 245);     // #EBEEF5
        inline constexpr Rgb888 TextDim = Rgb888(150, 158, 178);  // #969EB2
        inline constexpr Rgb888 TextStrong = Rgb888(255, 255, 255); // search query, selected key
        inline constexpr Rgb888 Pill = Rgb888(44, 50, 68);        // #2C3244
        inline constexpr Rgb888 BatteryGood = Rgb888(120, 230, 140);
        inline constexpr Rgb888 BatteryLow = Rgb888(255, 80, 80);
        inline constexpr Rgb888 Favorite = Rgb888(255, 196, 64);  // star of favorite games
        // search keyboard
        inline constexpr Rgb888 KeyTop = Rgb888(36, 41, 57);
        inline constexpr Rgb888 KeyBottom = Rgb888(29, 33, 46);
        inline constexpr Rgb888 KeySpecialTop = Rgb888(52, 59, 80);
        inline constexpr Rgb888 KeySpecialBottom = Rgb888(42, 48, 66);
        inline constexpr Rgb888 KeySelectedTop = Rgb888(66, 76, 104);
        inline constexpr Rgb888 KeySelectedBottom = Rgb888(46, 53, 74);
        inline constexpr Rgb888 FieldTop = Rgb888(17, 19, 26);
        inline constexpr Rgb888 FieldBottom = Rgb888(22, 25, 34);
        inline constexpr Rgb888 FieldStatus = Rgb888(17, 20, 27);
    }

    // The current colors: Defaults, changed by LoadFromThemeFolder when the theme is loaded.
    inline Rgb888 BgTop = Defaults::BgTop;
    inline Rgb888 BgBottom = Defaults::BgBottom;
    inline Rgb888 Tile = Defaults::Tile;
    inline Rgb888 TileDim = Defaults::TileDim;
    inline Rgb888 TileTop = Defaults::TileTop;
    inline Rgb888 TileSelectedTop = Defaults::TileSelectedTop;
    inline Rgb888 TileSelectedBottom = Defaults::TileSelectedBottom;
    /// The color unselected console tiles sink into, also gray level 0 of the tile and icon images.
    inline Rgb888 TileShade = Defaults::BgTop;
    inline Rgb888 Accent = Defaults::Accent;
    inline Rgb888 AccentDim = Defaults::AccentDim;
    /// Text on accent colored surfaces (the search key).
    inline Rgb888 OnAccent = Defaults::BgTop;
    inline Rgb888 Text = Defaults::Text;
    inline Rgb888 TextDim = Defaults::TextDim;
    inline Rgb888 TextStrong = Defaults::TextStrong;
    inline Rgb888 Pill = Defaults::Pill;
    inline Rgb888 BatteryGood = Defaults::BatteryGood;
    inline Rgb888 BatteryLow = Defaults::BatteryLow;
    inline Rgb888 Favorite = Defaults::Favorite;
    /// Gray level 0 of the top screen and cover console images (DsRunnerConsoleArt::Tint).
    inline Rgb888 ArtShade = Defaults::BgTop;
    inline Rgb888 KeyTop = Defaults::KeyTop;
    inline Rgb888 KeyBottom = Defaults::KeyBottom;
    inline Rgb888 KeySpecialTop = Defaults::KeySpecialTop;
    inline Rgb888 KeySpecialBottom = Defaults::KeySpecialBottom;
    inline Rgb888 KeySelectedTop = Defaults::KeySelectedTop;
    inline Rgb888 KeySelectedBottom = Defaults::KeySelectedBottom;
    inline Rgb888 FieldTop = Defaults::FieldTop;
    inline Rgb888 FieldBottom = Defaults::FieldBottom;
    inline Rgb888 FieldStatus = Defaults::FieldStatus;

    /// Labels on console colored surfaces and the light end of the console images, in every theme.
    inline constexpr Rgb888 White = Rgb888(255, 255, 255);

    /// @brief Resets the colors to Defaults and reads "colors" from /_pico/themes/<folder>/theme.json.
    ///        Reads the sd card: call when the theme is created, before the io thread runs.
    void LoadFromThemeFolder(const char* themeFolderName);

    /// @brief Puts the current colors into Pico's Material color scheme (dialogs, chips, buttons).
    inline void ApplyToMaterialScheme(MaterialColorScheme& scheme)
    {
        scheme.primary = Accent;
        scheme.onPrimary = OnAccent;
        scheme.secondaryContainer = Tile;
        scheme.onSecondaryContainer = Text;
        scheme.tertiary = Accent;
        scheme.onTertiary = OnAccent;
        scheme.tertiaryContainer = Pill;
        scheme.onTertiaryContainer = Text;
        scheme.surfaceBright = Tile;
        scheme.inverseOnSurface = BgTop;
        scheme.onSurface = Text;
        scheme.onSurfaceVariant = TextDim;
        scheme.mainIconBg = Pill;
        scheme.surfaceContainerHighest = Pill;
        scheme.scrim = BgBottom;
        scheme.outline = Accent;
    }
}

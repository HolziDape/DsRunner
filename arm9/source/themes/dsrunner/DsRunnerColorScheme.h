#pragma once
#include "themes/material/MaterialColorScheme.h"

namespace DsRunnerColors
{
    using Rgb888 = Rgb<8, 8, 8>;

    inline constexpr Rgb888 BgTop = Rgb888(15, 17, 23);       // #0F1117
    inline constexpr Rgb888 BgBottom = Rgb888(24, 28, 38);    // #181C26
    inline constexpr Rgb888 Tile = Rgb888(35, 40, 56);        // #232838
    inline constexpr Rgb888 TileDim = Rgb888(28, 32, 45);     // 25% blend with BgTop
    inline constexpr Rgb888 Accent = Rgb888(76, 194, 255);    // #4CC2FF
    inline constexpr Rgb888 Text = Rgb888(235, 238, 245);     // #EBEEF5
    inline constexpr Rgb888 TextDim = Rgb888(150, 158, 178);  // #969EB2
    inline constexpr Rgb888 Pill = Rgb888(44, 50, 68);        // #2C3244
    inline constexpr Rgb888 White = Rgb888(255, 255, 255);
    inline constexpr Rgb888 BatteryGood = Rgb888(120, 230, 140);
    inline constexpr Rgb888 BatteryLow = Rgb888(255, 80, 80);

    inline void ApplyDarkScheme(MaterialColorScheme& scheme)
    {
        scheme.primary = Accent;
        scheme.onPrimary = BgTop;
        scheme.secondaryContainer = Tile;
        scheme.onSecondaryContainer = Text;
        scheme.tertiary = Accent;
        scheme.onTertiary = BgTop;
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

#pragma once
#include <string.h>
#include "DsRunnerConsoles.h"

class IFontRepository;

/// @brief The built-in console images of the DsRunner theme: /_pico/themes/<theme>/consoles/<key>.bmp
///        (top screen, 256x192), <key>-tile.bmp (3x2 grid, 64x64), <key>-icon.bmp (4x3 grid and the icon on
///        the top screen, 32x32 4 bpp) and <key>-cover.bmp (cover flow, 128x96), drawn by tools/console-art.
///        They are templates: gray palette colors are tinted with the console color (see Tint), so they fit
///        every console, also those of /_dsrunner/systems.json. The user's own images (/_dsrunner/art,
///        /_dsrunner/tiles, icon.bmp and cover.bmp in the folder) come first.
class DsRunnerConsoleArt
{
public:
    using Color = DsRunnerConsoles::Color;

    enum class ImageType
    {
        Top,
        Tile,
        Icon,
        Cover
    };

    struct ArtKey
    {
        /// Folder name, like in DsRunnerConsoles::kConsoles.
        const char* folderName;
        /// Base name of the images.
        const char* key;
    };

    // Keep in sync with ART_KEYS in tools/dsrunner-art/dsrunnerart/consoles.py.
    static constexpr ArtKey kArtKeys[] =
    {
        { "NDS", "NDS" }, { "DS", "NDS" },
        { "GBA", "GBA" }, { "GBC", "GBC" }, { "GB", "GB" },
        { "NES", "NES" }, { "FC", "NES" },
        { "SNES", "SNES" }, { "SFC", "SNES" },
        { "SMS", "SMS" }, { "GG", "GG" },
        { "GEN", "MD" }, { "MD", "MD" }, { "MEGADRIVE", "MD" },
        { "PCE", "PCE" },
        { "APPS", "APPS" }, { "HOMEBREW", "APPS" },
        { "\xE2\x98\x85 Favoriten", "FAV" },
    };
    /// Image of consoles of systems.json that have no built-in image.
    static constexpr const char* kCustomConsoleKey = "CART";
    /// Image of all other folders.
    static constexpr const char* kFolderKey = "FOLDER";

    /// @brief Remembers the theme folder and checks whether its console images exist.
    ///        Call when the theme is loaded, before the io thread runs (reads the sd card).
    void Initialize(const char* themeFolderName, const IFontRepository* fontRepository);

    /// @brief Forgets the theme (when it is destroyed).
    void Reset()
    {
        _available = false;
        _fontRepository = nullptr;
    }

    /// @brief Returns true when the DsRunner theme is active and has its console images.
    bool IsAvailable() const { return _available; }

    /// @brief The fonts of the DsRunner theme (for the labels of the tiles), nullptr when not available.
    const IFontRepository* GetFontRepository() const { return _fontRepository; }

    /// @brief Gets the image key of a folder: its built-in console, CART for other consoles, else FOLDER.
    static const char* GetKey(const char* folderName);

    /// @brief Builds the path of the built-in image of a folder (check IsAvailable() first).
    /// @return False when no theme is set or the path doesn't fit.
    bool GetImagePath(const char* folderName, ImageType type, char* result, u32 resultSize) const;

    /// @brief Gets the tile color of a folder (the color its tile, icon and cover are tinted with).
    static Color GetTileColor(const char* folderName)
    {
        const auto* console = DsRunnerConsoles::FindByFolderName(folderName);
        return console ? console->tile : DsRunnerConsoles::kUnknownTile;
    }

    /// @brief Tints a template color: gray levels map 0 -> the theme background, 128 -> \p color,
    ///        255 -> white. Colors that are not gray stay as they are.
    static Color Tint(const Color& templateColor, const Color& color);

    /// @brief Converts a BMP palette (BGRA) to the DS format (BGR555), tinted with \p tint when given.
    static void ConvertPalette(const u8* bmpPalette, u32 count, const Color* tint, u16* result);

private:
    bool _available = false;
    const IFontRepository* _fontRepository = nullptr;
    // "/_pico/themes/<theme>/consoles/"
    char _folder[96] = { 0 };
};

extern DsRunnerConsoleArt gDsRunnerConsoleArt;

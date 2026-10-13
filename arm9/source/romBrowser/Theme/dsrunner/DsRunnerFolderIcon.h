#pragma once
#include "romBrowser/FileType/FileIcon.h"
#include "core/math/Rgb.h"

class IFontRepository;

/// @brief Icon of a folder in the DsRunner theme: a rounded badge in the console's color
///        with the console label ("NDS", "GBA", ...) on it.
class DsRunnerFolderIcon : public FileIcon
{
public:
    /// @brief Creates a badge for a folder. Known console folder names get the console's color and label,
    ///        everything else a neutral badge with the first letters of the name.
    DsRunnerFolderIcon(const TCHAR* name, const IFontRepository* fontRepository);

    /// @brief Creates a badge for a file type with the given label and color (e.g. "GBA" files).
    DsRunnerFolderIcon(const char* label, const Rgb<8, 8, 8>& color, const IFontRepository* fontRepository);

    void UploadGraphics() override;
    void Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor) override;

private:
    void SetLabel(const char* label);

    const IFontRepository* _fontRepository;
    Rgb<8, 8, 8> _badgeColor;
    char16_t _label[6];
};

#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "core/math/RgbMixer.h"
#include "fat/ff.h"
#include "DsRunnerColorScheme.h"
#include "DsRunnerConsoleArt.h"

DsRunnerConsoleArt gDsRunnerConsoleArt;

static const char* imageSuffix(DsRunnerConsoleArt::ImageType type)
{
    switch (type)
    {
        case DsRunnerConsoleArt::ImageType::Tile:
            return "-tile";
        case DsRunnerConsoleArt::ImageType::Icon:
            return "-icon";
        case DsRunnerConsoleArt::ImageType::Cover:
            return "-cover";
        default:
            return "";
    }
}

void DsRunnerConsoleArt::Initialize(const char* themeFolderName, const IFontRepository* fontRepository)
{
    _fontRepository = fontRepository;

    // the generic folder icon stands for the whole set: without it the old badges stay.
    // Themes without their own consoles folder (the color variants) share the one of the dsrunner theme.
    const char* folders[] = { themeFolderName, kSharedThemeFolder };
    for (const char* folder : folders)
    {
        mini_snprintf(_folder, sizeof(_folder), "/_pico/themes/%s/consoles/", folder);
        char path[128];
        FILINFO fileInfo;
        _available = GetImagePath("", ImageType::Icon, path, sizeof(path)) && f_stat(path, &fileInfo) == FR_OK;
        if (_available)
            break;
    }
    LOG_INFO("DsRunner: console images %s%s\n", _available ? "found in " : "not found", _available ? _folder : "");
}

const char* DsRunnerConsoleArt::GetKey(const char* folderName)
{
    for (const auto& artKey : kArtKeys)
    {
        if (!strcasecmp(artKey.folderName, folderName))
            return artKey.key;
    }
    return DsRunnerConsoles::FindByFolderName(folderName) ? kCustomConsoleKey : kFolderKey;
}

bool DsRunnerConsoleArt::GetImagePath(const char* folderName, ImageType type, char* result, u32 resultSize) const
{
    if (_folder[0] == 0)
        return false;
    // no _available check: Initialize uses this to find out
    int length = mini_snprintf(result, resultSize, "%s%s%s.bmp", _folder, GetKey(folderName), imageSuffix(type));
    return length > 0 && (u32)length < resultSize;
}

DsRunnerConsoleArt::Color DsRunnerConsoleArt::Tint(const Color& templateColor, const Color& color, const Color& shade)
{
    if (templateColor.r != templateColor.g || templateColor.g != templateColor.b)
        return templateColor;
    int level = templateColor.r;
    if (level <= 128)
        return RgbMixer::Lerp(shade, color, level, 128);
    return RgbMixer::Lerp(color, DsRunnerColors::White, level - 128, 127);
}

void DsRunnerConsoleArt::ConvertPalette(const u8* bmpPalette, u32 count, const Color* tint, u16* result,
    const Color& shade)
{
    for (u32 i = 0; i < count; i++)
    {
        Color color(bmpPalette[i * 4 + 2], bmpPalette[i * 4 + 1], bmpPalette[i * 4 + 0]);
        if (tint)
            color = Tint(color, *tint, shade);
        result[i] = (color.r >> 3) | ((color.g >> 3) << 5) | ((color.b >> 3) << 10);
    }
}

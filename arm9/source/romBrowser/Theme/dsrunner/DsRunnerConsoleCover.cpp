#include "common.h"
#include <string.h>
#include <memory>
#include <nds/arm9/cache.h>
#include <libtwl/dma/dmaNitro.h>
#include "fat/File.h"
#include "romBrowser/FileType/BmpHeader.h"
#include "themes/dsrunner/DsRunnerConsoleArt.h"
#include "DsRunnerConsoleCover.h"

DsRunnerConsoleCover* DsRunnerConsoleCover::Create(const char* folderName)
{
    char path[128];
    if (!gDsRunnerConsoleArt.IsAvailable() ||
        !gDsRunnerConsoleArt.GetImagePath(folderName, DsRunnerConsoleArt::ImageType::Cover, path, sizeof(path)))
    {
        return nullptr;
    }
    auto cover = std::unique_ptr<DsRunnerConsoleCover>(new DsRunnerConsoleCover());
    if (!cover->Load(path, folderName))
        return nullptr;
    return cover.release();
}

bool DsRunnerConsoleCover::Load(const char* path, const char* folderName)
{
    auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return false;

    // like BmpFileCover: 128x96, 8 bpp, the header and palette are read into the pixel buffer first
    if (!file->ReadExact(_coverBuffer, 0x436) || !BmpHeader::Validate(_coverBuffer, 128, 96, 8))
        return false;
    u32 dataOffset = _coverBuffer[0xA] | (_coverBuffer[0xB] << 8) | (_coverBuffer[0xC] << 16) | (_coverBuffer[0xD] << 24);
    auto tint = DsRunnerConsoleArt::GetTileColor(folderName);
    DsRunnerConsoleArt::ConvertPalette(&_coverBuffer[0x36], 256, &tint, _palette);
    if (file->Seek(dataOffset) != FR_OK || !file->ReadExact(_coverBuffer, sizeof(_coverBuffer)))
        return false;
    DC_FlushRange(_coverBuffer, sizeof(_coverBuffer));
    DC_FlushRange(_palette, sizeof(_palette));
    return true;
}

void DsRunnerConsoleCover::Upload2DCoverBitmap(void* destination) const
{
    cover_bitmapToTiledCopy(_coverBuffer, destination);
}

void DsRunnerConsoleCover::Upload2DCoverPalette(void* destination) const
{
    dma_ntrCopy32(3, _palette, destination, sizeof(_palette));
}

#pragma once
#include "romBrowser/FileType/FileCover.h"

/// @brief Cover of a folder in the cover flow: the built-in image of its console (DsRunnerConsoleArt),
///        128x96 8 bpp, tinted in the console's color.
class alignas(32) DsRunnerConsoleCover : public FileCover
{
public:
    /// @brief Loads the cover of the folder \p folderName. Call on the io thread.
    /// @return The cover, or nullptr when there are no console images or it can't be loaded.
    static DsRunnerConsoleCover* Create(const char* folderName);

    VBlankTextureLoadRequest CreateTextureLoadRequest() const override
    {
        return VBlankTextureLoadRequest(
            _coverBuffer, sizeof(_coverBuffer), _texVramOffset,
            _palette, sizeof(_palette), _plttVramOffset,
            nullptr, nullptr);
    }

    void Upload2DCoverBitmap(void* destination) const override;
    void Upload2DCoverPalette(void* destination) const override;

    // a placeholder like Pico's folder cover, not a picture of the folder
    bool IsActualCover() const override { return false; }

private:
    u8 _coverBuffer[128 * 96] alignas(32);
    u16 _palette[256] alignas(32);

    DsRunnerConsoleCover() { }
    bool Load(const char* path, const char* folderName);
};

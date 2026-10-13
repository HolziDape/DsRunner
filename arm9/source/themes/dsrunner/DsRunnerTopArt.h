#pragma once
#include "core/task/TaskQueue.h"

/// @brief Full screen artwork on the top screen (256x192, 8 bpp BMP from /_dsrunner/art).
///        Shown on sub BG2 above the gradient. Changing the art fades the old one out to the gradient
///        and the new one in, the lower part with the info text is always darkened.
///        The blending is done per screen line with an HBlank DMA to the sub engine's BLDALPHA register.
class DsRunnerTopArt
{
public:
    DsRunnerTopArt();
    ~DsRunnerTopArt();

    /// @brief Shows the artwork at \p path, or none for nullptr / an empty path.
    ///        A missing or invalid file shows no artwork (the gradient stays visible).
    void Request(const char* path, TaskQueueBase* ioTaskQueue);

    /// @brief Advances loading and fading. Call once per frame.
    void Update();

    /// @brief Uploads a loaded artwork and programs the display. Call in vblank.
    void VBlank();

private:
    enum class LoadResult
    {
        None,
        Ok,
        Failed
    };

    char _requestedPath[256] = { 0 };
    TaskQueueBase* _ioTaskQueue = nullptr;
    // frames to wait before loading, so holding the dpad doesn't read every file on the way
    int _loadDelay = 0;
    QueueTask<void> _loadTask;
    volatile LoadResult _loadResult = LoadResult::None;
    bool _uploadPending = false;
    bool _artInVram = false;
    // 0 = invisible, 16 = fully visible
    int _fade = 0;
    int _fadeTarget = 0;
    int _tableFade = -1;
    bool _hblankDmaRunning = false;

    alignas(32) u8 _pixels[256 * 192];
    alignas(32) u16 _palette[256];
    // BLDALPHA value per screen line, plus one because the HBlank DMA always writes the next line
    alignas(32) u16 _blendTable[193];

    void StartLoad();
    LoadResult Load(const char* path, const vu8& cancelRequested);
    void UpdateBlendTable();
    void StopBlending();
};

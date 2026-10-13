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

    // the art that should be shown (main thread only)
    char _requestedPath[256] = { 0 };
    // the file the load task reads, only changed while no load task exists
    char _loadingPath[256] = { 0 };
    TaskQueueBase* _ioTaskQueue = nullptr;
    // frames to wait before loading, so holding the dpad doesn't read every file on the way
    int _loadDelay = 0;
    // a requested art still has to be loaded
    bool _loadNeeded = false;
    // At most one load is queued or running at a time (more could fill the io queue on a slow
    // card). Every request gets a new number; a load for an older one stops early and is dropped.
    QueueTask<void> _loadTask;
    volatile u32 _requestNumber = 0;
    volatile u32 _loadedNumber = 0;
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
    // load buffers: kept here instead of on the small io thread stack
    alignas(4) u8 _bmpPalette[256 * 4];
    alignas(4) u8 _rowBuffer[256];
    // BLDALPHA value per screen line, plus one because the HBlank DMA always writes the next line
    alignas(32) u16 _blendTable[193];

    void StartLoad();
    LoadResult Load(const char* path, u32 requestNumber, const vu8& cancelRequested);
    bool IsStale(u32 requestNumber, const vu8& cancelRequested) const
    {
        return cancelRequested || requestNumber != _requestNumber;
    }
    void UpdateBlendTable();
    void StopBlending();
};

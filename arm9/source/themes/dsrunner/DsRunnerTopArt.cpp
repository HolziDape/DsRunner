#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/dma/dmaNitro.h>
#include "fat/File.h"
#include "VBlank.h"
#include "DsRunnerConsoleArt.h"
#include "DsRunnerTopArt.h"

#define ART_WIDTH           256
#define ART_HEIGHT          192
// the bitmap lives in sub bg vram (bank C) behind the cover and gradient data
#define ART_VRAM_OFFSET     0x10000
// the BMP header up to and including the BITMAPINFOHEADER
#define BMP_HEADER_SIZE     54

// longer than the key repeat delay (8 frames), so scrolling with a held dpad loads nothing
#define LOAD_DELAY_FRAMES   20
// the pixels are read in parts, so a load that isn't needed anymore stops early
#define READ_CHUNK_SIZE     8192
#define FADE_STEP           2

// art visibility per screen line (out of 16): fully visible at the top, then a long soft
// (smoothstep) fade towards the info text (see DsRunnerFileInfoCardView, the title starts at line 122)
#define DARKEN_START_Y      64
#define DARKEN_END_Y        140
#define DARKEN_MIN          5

// the blending runs on its own dma channel: channel 3 does the copies, channel 0 is stopped every vblank
#define HBLANK_DMA_CHANNEL  1

#define BLDCNT_ALPHA        (1 << 6)
#define BLDCNT_A_BG2        (1 << 2)
#define BLDCNT_B_BG0        (1 << 8)
#define BLDCNT_B_BACKDROP   (1 << 13)

static inline u32 readLe32(const u8* data)
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

DsRunnerTopArt::DsRunnerTopArt() { }

DsRunnerTopArt::~DsRunnerTopArt()
{
    if (_loadTask.IsValid())
    {
        // the io thread may be writing into _pixels right now: make it stop and wait for it
        _requestNumber = _requestNumber + 1;
        while (!_loadTask.GetTask().IsCompleted())
            VBlank::Wait();
        _loadTask.Dispose();
    }
    StopBlending();
    REG_DISPCNT_SUB &= ~(1 << 10);
    REG_BLDCNT_SUB = 0;
}

void DsRunnerTopArt::Request(const char* path, const char* fallbackPath, const DsRunnerConsoles::Color& fallbackTint,
    TaskQueueBase* ioTaskQueue)
{
    if (!path)
        path = "";
    if (!fallbackPath)
        fallbackPath = "";
    if (strcmp(path, _requestedPath) == 0 && strcmp(fallbackPath, _requestedFallbackPath) == 0 &&
        fallbackTint.r == _requestedTint.r && fallbackTint.g == _requestedTint.g && fallbackTint.b == _requestedTint.b)
    {
        return;
    }

    strlcpy(_requestedPath, path, sizeof(_requestedPath));
    strlcpy(_requestedFallbackPath, fallbackPath, sizeof(_requestedFallbackPath));
    _requestedTint = fallbackTint;
    _ioTaskQueue = ioTaskQueue;
    // a running load notices the new number and stops; the next one starts when it is done
    _requestNumber = _requestNumber + 1;
    _loadNeeded = path[0] != 0 || fallbackPath[0] != 0;
    _uploadPending = false;
    _loadDelay = LOAD_DELAY_FRAMES;
    // fade out the current art, the new one fades in once it's loaded
    _fadeTarget = 0;
}

void DsRunnerTopArt::Update()
{
    if (_loadDelay > 0)
        _loadDelay--;

    if (_loadTask.IsValid() && _loadTask.GetTask().IsCompleted())
    {
        _loadTask.Dispose();
        if (_loadedNumber == _requestNumber && _loadResult == LoadResult::Ok)
        {
            // uploaded in vblank as soon as the old art is faded out
            _uploadPending = true;
        }
    }

    if (_loadNeeded && _loadDelay == 0 && !_loadTask.IsValid() && _ioTaskQueue)
    {
        _loadNeeded = false;
        StartLoad();
    }

    if (_fade < _fadeTarget)
        _fade = std::min(_fade + FADE_STEP, _fadeTarget);
    else if (_fade > _fadeTarget)
        _fade = std::max(_fade - FADE_STEP, _fadeTarget);

    if (_fade != _tableFade)
    {
        UpdateBlendTable();
    }
}

void DsRunnerTopArt::StartLoad()
{
    u32 requestNumber = _requestNumber;
    strlcpy(_loadingPath, _requestedPath, sizeof(_loadingPath));
    strlcpy(_loadingFallbackPath, _requestedFallbackPath, sizeof(_loadingFallbackPath));
    _loadingTint = _requestedTint;
    _loadTask = _ioTaskQueue->Enqueue([this, requestNumber] (const vu8& cancelRequested)
    {
        // the user's own artwork first, then the built-in console image
        LoadResult result = LoadResult::Failed;
        if (_loadingPath[0] != 0)
            result = Load(_loadingPath, nullptr, requestNumber, cancelRequested);
        if (result == LoadResult::Failed && _loadingFallbackPath[0] != 0)
            result = Load(_loadingFallbackPath, &_loadingTint, requestNumber, cancelRequested);
        _loadResult = result;
        _loadedNumber = requestNumber;
        return TaskResult<void>::Completed();
    });
}

DsRunnerTopArt::LoadResult DsRunnerTopArt::Load(const char* path, const DsRunnerConsoles::Color* tint,
    u32 requestNumber, const vu8& cancelRequested)
{
    if (IsStale(requestNumber, cancelRequested))
        return LoadResult::None;

    auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return LoadResult::Failed;

    // 256x192, 8 bits per pixel, uncompressed, rows bottom-up or top-down
    alignas(4) u8 header[BMP_HEADER_SIZE];
    if (!file->ReadExact(header, sizeof(header)) || header[0] != 'B' || header[1] != 'M')
    {
        LOG_DEBUG("Art %s: not a bmp\n", path);
        return LoadResult::Failed;
    }
    u32 pixelOffset = readLe32(&header[0x0A]);
    u32 dibSize = readLe32(&header[0x0E]);
    u32 width = readLe32(&header[0x12]);
    s32 height = (s32)readLe32(&header[0x16]);
    u32 bpp = header[0x1C] | (header[0x1D] << 8);
    u32 compression = readLe32(&header[0x1E]);
    u32 colors = readLe32(&header[0x2E]);
    if (colors == 0 || colors > 256)
        colors = 256;
    if (dibSize < 40 || width != ART_WIDTH || (height != ART_HEIGHT && height != -ART_HEIGHT) ||
        bpp != 8 || compression != 0)
    {
        LOG_DEBUG("Art %s: needs to be a 256x192 8 bpp uncompressed bmp\n", path);
        return LoadResult::Failed;
    }

    u8* bmpPalette = _bmpPalette;
    memset(bmpPalette, 0, sizeof(_bmpPalette));
    if (file->Seek(14 + dibSize) != FR_OK || !file->ReadExact(bmpPalette, colors * 4))
        return LoadResult::Failed;
    if (IsStale(requestNumber, cancelRequested) || file->Seek(pixelOffset) != FR_OK)
        return LoadResult::None;
    for (u32 offset = 0; offset < sizeof(_pixels); offset += READ_CHUNK_SIZE)
    {
        if (IsStale(requestNumber, cancelRequested))
            return LoadResult::None;
        if (!file->ReadExact(&_pixels[offset], std::min<u32>(READ_CHUNK_SIZE, sizeof(_pixels) - offset)))
            return LoadResult::Failed;
    }

    if (height > 0)
    {
        // bottom-up: flip the rows
        u8* row = _rowBuffer;
        for (int y = 0; y < ART_HEIGHT / 2; y++)
        {
            u8* top = &_pixels[y * ART_WIDTH];
            u8* bottom = &_pixels[(ART_HEIGHT - 1 - y) * ART_WIDTH];
            memcpy(row, top, ART_WIDTH);
            memcpy(top, bottom, ART_WIDTH);
            memcpy(bottom, row, ART_WIDTH);
        }
    }

    // BGRA -> BGR555, the built-in console images are tinted with the console color
    DsRunnerConsoleArt::ConvertPalette(bmpPalette, 256, tint, _palette);

    // Color 0 of a bitmap background is transparent: draw those pixels with the closest other color.
    int closest = 1;
    int closestDistance = 0x7FFFFFFF;
    for (u32 i = 1; i < colors; i++)
    {
        int dr = (int)(_palette[i] & 31) - (int)(_palette[0] & 31);
        int dg = (int)((_palette[i] >> 5) & 31) - (int)((_palette[0] >> 5) & 31);
        int db = (int)((_palette[i] >> 10) & 31) - (int)((_palette[0] >> 10) & 31);
        int distance = dr * dr + dg * dg + db * db;
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closest = i;
        }
    }
    if (closestDistance != 0 && colors < 256)
    {
        // a free palette entry can take the exact color
        closest = colors;
        _palette[closest] = _palette[0];
    }
    u32* pixels32 = (u32*)_pixels;
    for (u32 i = 0; i < sizeof(_pixels) / 4; i++)
    {
        u32 quad = pixels32[i];
        // only touch words that have a zero byte
        if ((quad - 0x01010101) & ~quad & 0x80808080)
        {
            for (int shift = 0; shift < 32; shift += 8)
            {
                if (((quad >> shift) & 0xFF) == 0)
                    quad |= closest << shift;
            }
            pixels32[i] = quad;
        }
    }

    // the vblank code copies these with dma
    DC_FlushRange(_pixels, sizeof(_pixels));
    DC_FlushRange(_palette, sizeof(_palette));
    return LoadResult::Ok;
}

void DsRunnerTopArt::UpdateBlendTable()
{
    _tableFade = _fade;
    for (int y = 0; y <= ART_HEIGHT; y++)
    {
        int visibility = 16;
        if (y >= DARKEN_END_Y)
            visibility = DARKEN_MIN;
        else if (y > DARKEN_START_Y)
            visibility = 16 - (16 - DARKEN_MIN) * (y - DARKEN_START_Y) / (DARKEN_END_Y - DARKEN_START_Y);
        int eva = visibility * _fade / 16;
        _blendTable[y] = eva | ((16 - eva) << 8);
    }
    DC_FlushRange(_blendTable, sizeof(_blendTable));
}

void DsRunnerTopArt::StopBlending()
{
    if (_hblankDmaRunning)
    {
        dma_ntrStopSafe(HBLANK_DMA_CHANNEL);
        _hblankDmaRunning = false;
    }
}

void DsRunnerTopArt::VBlank()
{
    if (_uploadPending && _fade == 0)
    {
        _uploadPending = false;
        REG_DISPCNT_SUB &= ~(1 << 10);
        dma_ntrCopy32(3, _pixels, (u8*)GFX_BG_SUB + ART_VRAM_OFFSET, sizeof(_pixels));
        // color 0 is the backdrop, the art doesn't use it (the bg palette is free otherwise:
        // the gradient and the cover use extended palettes)
        dma_ntrCopy16(3, &_palette[1], &GFX_PLTT_BG_SUB[1], 255 * 2);
        _artInVram = true;
        _fadeTarget = 16;
    }

    // the splash screen fade at startup uses the blending registers itself (and sub BG1)
    bool splashVisible = REG_DISPCNT_SUB & (1 << 9);
    if (!_artInVram || _fade == 0 || splashVisible)
    {
        if (!splashVisible)
        {
            StopBlending();
            REG_DISPCNT_SUB &= ~(1 << 10);
            REG_BLDCNT_SUB = 0;
        }
        return;
    }

    // 256 color bitmap at ART_VRAM_OFFSET, same priority as the text sprites so they stay on top
    REG_BG2CNT_SUB = (1 << 7) | ((ART_VRAM_OFFSET / 0x4000) << 8) | (1 << 14) | 2;
    REG_BG2PA_SUB = 0x100;
    REG_BG2PB_SUB = 0;
    REG_BG2PC_SUB = 0;
    REG_BG2PD_SUB = 0x100;
    REG_BG2X_SUB = 0;
    REG_BG2Y_SUB = 0;
    REG_DISPCNT_SUB |= 1 << 10;

    REG_BLDCNT_SUB = BLDCNT_ALPHA | BLDCNT_A_BG2 | BLDCNT_B_BG0 | BLDCNT_B_BACKDROP;
    // line 0 directly, the HBlank DMA writes the value of the next line at the end of every line
    StopBlending();
    REG_BLDALPHA_SUB = _blendTable[0];
    dma_ntrSetParams(HBLANK_DMA_CHANNEL, &_blendTable[1], &REG_BLDALPHA_SUB,
        DMACNT_ENABLE | DMACNT_MODE_HBLANK | DMACNT_REPEAT | DMACNT_16BIT |
        DMACNT_SRC_MODE_INCREMENT | DMACNT_DST_MODE_FIXED | DMACNT_COUNT(1));
    _hblankDmaRunning = true;
}

#pragma once
#include "gui/views/View.h"
#include "core/SharedPtr.h"
#include "rtcIpc.h"

class StatusBarView : public View
{
    SHARED_ONLY(StatusBarView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 64, 16);
    }

private:
    StatusBarView();

    void RedrawBitmap(int hours, int minutes, u16 batteryState);
    void PutPixel(int x, int y, u8 color);
    void DrawChar(int x, int y, char c, u8 color);

    u8 _tileBuffer[512] alignas(4);
    bool _needsUpload = true;
    u32 _vramOffset = 0;
    vu16* _vramAddress = nullptr;

    int _lastHour = -1;
    int _lastMinute = -1;
    u16 _lastBattery = 0xFFFF;
    u32 _framesSinceRtcRead = 0;
};

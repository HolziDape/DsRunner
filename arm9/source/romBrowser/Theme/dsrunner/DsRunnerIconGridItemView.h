#pragma once
#include "romBrowser/views/IconGridItemView.h"

class MaterialColorScheme;

class DsRunnerIconGridItemView : public IconGridItemView
{
    SHARED_ONLY(DsRunnerIconGridItemView)

public:
    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x - 2, _position.y - 2, 48, 48);
    }

    void Draw(GraphicsContext& graphicsContext) override;

    void SetGraphics(const VramToken& vramToken) override
    {
        _bgVramOffset = vramToken.GetVramOffset();
    }

    static VramToken UploadGraphics(const VramContext& vramContext);

private:
    const MaterialColorScheme* _colorScheme;
    u32 _bgVramOffset = 0;

    DsRunnerIconGridItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel,
        const MaterialColorScheme* colorScheme);
};

#pragma once
#include "gui/views/LabelView.h"
#include "romBrowser/views/BannerListItemView.h"

class MaterialColorScheme;
class IFontRepository;

class DsRunnerBannerListItemView : public BannerListItemView
{
    SHARED_ONLY(DsRunnerBannerListItemView)

public:
    void Draw(GraphicsContext& graphicsContext) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 203, 44);
    }

    void SetGraphics(const VramToken& vramToken) override
    {
        _bgVramOffset = vramToken.GetVramOffset();
    }

    static VramToken UploadGraphics(const VramContext& vramContext);

private:
    const MaterialColorScheme* _colorScheme;
    u32 _bgVramOffset = 0;

    DsRunnerBannerListItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel,
        const MaterialColorScheme* colorScheme, const IFontRepository* fontRepository);
};

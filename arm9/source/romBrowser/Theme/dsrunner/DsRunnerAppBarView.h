#pragma once
#include "romBrowser/views/AppBarView.h"
#include "gui/views/Label2DView.h"

class MaterialColorScheme;
class IFontRepository;

class DsRunnerAppBarView : public AppBarView
{
    SHARED_ONLY(DsRunnerAppBarView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    void SetTitle(const char* title)
    {
        if (_titleLabel)
            _titleLabel->SetText(title);
    }

private:
    SharedPtr<Label2DView> _titleLabel;
    const MaterialColorScheme* _colorScheme;

    DsRunnerAppBarView(int x, int y, Orientation orientation,
        int startButtonCount, int endButtonCount,
        const MaterialColorScheme* colorScheme,
        const IFontRepository* fontRepository);
};

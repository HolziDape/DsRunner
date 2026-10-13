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

    void SetTitle(const char* title) override;

private:
    SharedPtr<Label2DView> _titleLabel;
    const MaterialColorScheme* _colorScheme;
    // title that is currently set on the label, so it is only rendered again when it changes
    char _title[64] = { 0 };

    DsRunnerAppBarView(int x, int y, Orientation orientation,
        int startButtonCount, int endButtonCount,
        const MaterialColorScheme* colorScheme,
        const IFontRepository* fontRepository);
};

#include "common.h"
#include "gui/VramContext.h"
#include "romBrowser/views/IconButton2DView.h"
#include "themes/IFontRepository.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "DsRunnerAppBarView.h"

DsRunnerAppBarView::DsRunnerAppBarView(int x, int y, Orientation orientation,
    int startButtonCount, int endButtonCount,
    const MaterialColorScheme* colorScheme,
    const IFontRepository* fontRepository)
    : AppBarView(x, y, orientation, startButtonCount, endButtonCount, colorScheme)
    , _titleLabel(fontRepository ? Label2DView::CreateShared(120, 16, 64, fontRepository->GetFont(FontType::Medium10)) : nullptr)
    , _colorScheme(colorScheme)
{
    for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
    {
        _buttons[i] = IconButton2DView::CreateShared(
            IconButtonView::Type::Standard,
            IconButtonView::State::NoToggle,
            md::sys::color::inverseOnSurface,
            colorScheme);
        AddChildTail(_buttons[i].GetPointer());
    }

    if (_titleLabel)
    {
        _titleLabel->SetText("Konsolen");
        AddChildTail(_titleLabel.GetPointer());
    }
}

void DsRunnerAppBarView::InitVram(const VramContext& vramContext)
{
    AppBarView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        auto iconButtonVramToken = IconButton2DView::UploadGraphics(*objVramManager);
        for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
        {
            static_cast<IconButton2DView*>(_buttons[i].GetPointer())->SetGraphics(iconButtonVramToken);
        }
    }
}

void DsRunnerAppBarView::Update()
{
    AppBarView::Update();

    if (_titleLabel)
    {
        int titleX = (_startButtonCount > 0) ? 34 : 10;
        _titleLabel->SetPosition(titleX, _position.y + 7);
    }
}

void DsRunnerAppBarView::Draw(GraphicsContext& graphicsContext)
{
    if (_titleLabel)
    {
        _titleLabel->SetForegroundColor(DsRunnerColors::Text);
        _titleLabel->SetBackgroundColor(DsRunnerColors::BgTop);
    }

    AppBarView::Draw(graphicsContext);
}

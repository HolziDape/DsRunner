#include "common.h"
#include <string.h>
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
    , _titleLabel(fontRepository ? Label2DView::CreateShared(180, 16, 64, fontRepository->GetFont(FontType::Medium10)) : nullptr)
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
        // long titles (search results: "Suche: <query> · <console> (<count>)") end with "…"
        _titleLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
        AddChildTail(_titleLabel.GetPointer());
    }
}

void DsRunnerAppBarView::SetTitle(const char* title)
{
    if (!_titleLabel || strncmp(_title, title, sizeof(_title)) == 0)
        return;
    strlcpy(_title, title, sizeof(_title));
    _titleLabel->SetText(_title);
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
        if (_orientation == Orientation::Vertical)
        {
            // the vertical app bar is a column of buttons at the left, there is no room for a title
            _titleLabel->SetPosition(0, -32);
        }
        else
        {
            int titleX = (_startButtonCount > 0) ? 34 : 10;
            _titleLabel->SetPosition(titleX, _position.y + 7);
        }
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

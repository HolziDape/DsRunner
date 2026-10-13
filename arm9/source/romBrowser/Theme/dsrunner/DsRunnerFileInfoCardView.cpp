#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "DsRunnerFileInfoCardView.h"

DsRunnerFileInfoCardView::DsRunnerFileInfoCardView(const MaterialColorScheme* colorScheme,
    const IFontRepository* fontRepository)
    : _firstLine(Label2DView::CreateShared(232, 16, 128, fontRepository->GetFont(FontType::Medium11)))
    , _secondLine(Label2DView::CreateShared(232, 16, 128, fontRepository->GetFont(FontType::Regular10)))
    , _thirdLine(Label2DView::CreateShared(232, 16, 128, fontRepository->GetFont(FontType::Regular10)))
    , _filenameLabelView(Label2DView::CreateShared(232, 16, 256, fontRepository->GetFont(FontType::Medium7_5)))
    , _colorScheme(colorScheme)
{
    AddChildTail(_firstLine.GetPointer());
    AddChildTail(_secondLine.GetPointer());
    _filenameLabelView->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    AddChildTail(_filenameLabelView.GetPointer());
}

void DsRunnerFileInfoCardView::InitVram(const VramContext& vramContext)
{
    BannerView::InitVram(vramContext);
}

void DsRunnerFileInfoCardView::Update()
{
    BannerView::Update();
    // Title at (12, 192 - 46) = (12, 146)
    _firstLine->SetPosition(_position.x + 12, _position.y + 146);
    // Subtitle at (12, 192 - 24) = (12, 168)
    _secondLine->SetPosition(_position.x + 12, _position.y + 168);
    _filenameLabelView->SetPosition(_position.x + 12, _position.y + 168);

    if (_icon)
    {
        _icon->SetPosition(_position.x + 216, _position.y + 146);
        _icon->Update();
    }
}

void DsRunnerFileInfoCardView::Draw(GraphicsContext& graphicsContext)
{
    // Text colors matching mockup:
    _firstLine->SetForegroundColor(DsRunnerColors::Text);
    _firstLine->SetBackgroundColor(DsRunnerColors::BgTop);

    _secondLine->SetForegroundColor(DsRunnerColors::TextDim);
    _secondLine->SetBackgroundColor(DsRunnerColors::BgTop);

    _filenameLabelView->SetForegroundColor(DsRunnerColors::TextDim);
    _filenameLabelView->SetBackgroundColor(DsRunnerColors::BgTop);

    BannerView::Draw(graphicsContext);

    if (_icon)
    {
        _icon->Draw(graphicsContext, DsRunnerColors::Text);
    }
}

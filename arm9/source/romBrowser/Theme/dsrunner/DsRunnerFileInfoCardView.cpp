#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "themes/dsrunner/DsRunnerHeroState.h"
#include "DsRunnerFileInfoCardView.h"

// The top screen background is a gradient that ends in a flat DsRunnerColors::BgTop band
// (see DsRunnerSubBackground). The info text lives in that band.
#define TEXT_X          12
#define TITLE_Y         122
#define SECOND_LINE_Y   140
#define THIRD_LINE_Y    155
#define FILE_NAME_Y     173
#define ICON_X          216
#define ICON_Y          124
// text stops left of the icon
#define TEXT_WIDTH      192

DsRunnerFileInfoCardView::DsRunnerFileInfoCardView(const MaterialColorScheme* colorScheme,
    const IFontRepository* fontRepository, DsRunnerHeroState* heroState)
    : _firstLine(Label2DView::CreateShared(TEXT_WIDTH, 16, 128, fontRepository->GetFont(FontType::Medium11)))
    , _secondLine(Label2DView::CreateShared(TEXT_WIDTH, 16, 128, fontRepository->GetFont(FontType::Regular10)))
    , _thirdLine(Label2DView::CreateShared(TEXT_WIDTH, 16, 128, fontRepository->GetFont(FontType::Regular10)))
    , _filenameLabelView(Label2DView::CreateShared(232, 16, 256, fontRepository->GetFont(FontType::Medium7_5)))
    , _colorScheme(colorScheme)
    , _heroState(heroState)
{
    AddChildTail(_firstLine.GetPointer());
    AddChildTail(_secondLine.GetPointer());
    AddChildTail(_thirdLine.GetPointer());
    _filenameLabelView->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    AddChildTail(_filenameLabelView.GetPointer());
}

void DsRunnerFileInfoCardView::InitVram(const VramContext& vramContext)
{
    BannerView::InitVram(vramContext);
}

void DsRunnerFileInfoCardView::SetFileNameAsync(TaskQueueBase* taskQueue, const TCHAR* fileName, bool useAsTitle)
{
    // the top screen gradient follows the console of the selected item
    _heroState->SetColor(DsRunnerConsoles::GetHeroColor(fileName));

    const auto* console = useAsTitle ? DsRunnerConsoles::FindByFolderName(fileName) : nullptr;
    if (console)
    {
        // a console folder: use its full name instead of the folder name as title
        _lines = 1;
        SetFirstLineAsync(taskQueue, console->fullName, true);
        SetSecondLineAsync(taskQueue, u"", 0);
        SetThirdLineAsync(taskQueue, u"", 0);
    }
    else
    {
        BannerView::SetFileNameAsync(taskQueue, fileName, useAsTitle);
    }

    if (taskQueue)
        _filenameLabelView->SetTextAsync(taskQueue, fileName);
    else
        _filenameLabelView->SetText(fileName);
}

void DsRunnerFileInfoCardView::Update()
{
    BannerView::Update();
    _firstLine->SetPosition(_position.x + TEXT_X, _position.y + TITLE_Y);
    _secondLine->SetPosition(_position.x + TEXT_X, _position.y + SECOND_LINE_Y);
    _thirdLine->SetPosition(_position.x + TEXT_X, _position.y + THIRD_LINE_Y);
    _filenameLabelView->SetPosition(_position.x + TEXT_X, _position.y + FILE_NAME_Y);

    if (_icon)
    {
        _icon->SetPosition(_position.x + ICON_X, _position.y + ICON_Y);
        _icon->Update();
    }
}

void DsRunnerFileInfoCardView::Draw(GraphicsContext& graphicsContext)
{
    _firstLine->SetForegroundColor(DsRunnerColors::Text);
    _firstLine->SetBackgroundColor(DsRunnerColors::BgTop);

    _secondLine->SetForegroundColor(DsRunnerColors::TextDim);
    _secondLine->SetBackgroundColor(DsRunnerColors::BgTop);

    _thirdLine->SetForegroundColor(DsRunnerColors::TextDim);
    _thirdLine->SetBackgroundColor(DsRunnerColors::BgTop);

    _filenameLabelView->SetForegroundColor(DsRunnerColors::TextDim);
    _filenameLabelView->SetBackgroundColor(DsRunnerColors::BgTop);

    BannerView::Draw(graphicsContext);

    if (_icon)
    {
        _icon->Draw(graphicsContext, DsRunnerColors::BgTop);
    }
}

#include "common.h"
#include <string.h>
#include <algorithm>
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "gui/GraphicsContext.h"
#include "services/settings/AppSettings.h"
#include "gui/VramContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "themes/dsrunner/DsRunnerConsoleArt.h"
#include "themes/dsrunner/DsRunnerHeroState.h"
#include "themes/dsrunner/DsRunnerText.h"
#include "DsRunnerLibrary.h"
#include "DsRunnerSearch.h"
#include "DsRunnerNames.h"
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

// artworks mirror the sd card: /Games/NDS/Mario.nds -> /_dsrunner/art/Games/NDS/Mario.bmp
#define ART_FOLDER      "/_dsrunner/art"

// English month names for "on Oct 3"; the other texts of the play statistics are in DsRunnerText
static constexpr const char* sMonthNames[] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
// up to this many days ago the date is shown as "vor N Tagen"
#define DAYS_AGO_MAX    6

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
    RequestArt(fileName);
    _pendingCountFolder[0] = 0;

    const auto* console = useAsTitle ? DsRunnerConsoles::FindByFolderName(fileName) : nullptr;
    if (console)
    {
        // a console folder: use its full name instead of the folder name as title
        _lines = 1;
        SetFirstLineAsync(taskQueue, DsRunnerText::Localize(console->fullName), true);
        SetSecondLineAsync(taskQueue, u"", 0);
        SetThirdLineAsync(taskQueue, u"", 0);
    }
    else if (_isFolder)
    {
        BannerView::SetFileNameAsync(taskQueue, fileName, useAsTitle);
    }
    else
    {
        // games without a banner title: the name without extension (and tags, settings.json "cleanNames")
        char displayName[sizeof(_fileName)];
        DsRunnerNames::GetDisplayName(fileName, displayName, sizeof(displayName));
        BannerView::SetFileNameAsync(taskQueue, displayName, useAsTitle);
    }

    if (console && DsRunnerSearch::IsEntry(fileName))
    {
        // the search tile: how to get there from anywhere
        char16_t hint[48];
        StringUtil::Copy(hint, DsRunnerText::Get(DsRunnerText::Id::SearchTileHint), sizeof(hint) / sizeof(hint[0]));
        SetThirdLineAsync(taskQueue, hint);
    }

    if (_isFolder && useAsTitle && !ShowGameCount(taskQueue, fileName) &&
        strlen(fileName) < sizeof(_pendingCountFolder))
    {
        // not counted yet: Update() fills it in later
        strcpy(_pendingCountFolder, fileName);
        _textTaskQueue = taskQueue;
    }

    strlcpy(_fileName, fileName, sizeof(_fileName));
    _fileNameTaskQueue = taskQueue;
    ShowPlayStats(taskQueue);
    UpdateFileNameLine();
}

void DsRunnerFileInfoCardView::ShowPlayStats(TaskQueueBase* taskQueue)
{
    // Games show how often and when they were started, in the first line below the title.
    // Banner titles with three lines leave no room, then it goes in front of the file name.
    _playStatsText[0] = 0;
    _playStatsOnFileNameLine = false;
    if (_isFolder)
        return;
    FormatPlayStats(_playStatsText, sizeof(_playStatsText));
    if (_lines >= 3)
    {
        _playStatsOnFileNameLine = true;
        return;
    }
    char16_t text16[sizeof(_playStatsText)];
    StringUtil::Copy(text16, _playStatsText, sizeof(text16) / sizeof(text16[0]));
    if (_lines <= 1)
        SetSecondLineAsync(taskQueue, text16);
    else
        SetThirdLineAsync(taskQueue, text16);
}

void DsRunnerFileInfoCardView::FormatPlayStats(char* text, u32 textSize) const
{
    const auto* entry = gDsRunnerLibrary.playStats.Find(_folderPath, _fileName);
    if (!entry || entry->count == 0)
    {
        bool inHistory = gDsRunnerLibrary.playHistory.GetRank(_folderPath, _fileName) >= 0;
        strlcpy(text, inHistory || entry ? DsRunnerText::Get(DsRunnerText::Id::StatsPlayedBefore)
            : DsRunnerText::Get(DsRunnerText::Id::StatsNeverPlayed), textSize);
        return;
    }
    u32 length = mini_snprintf(text, textSize, DsRunnerText::Get(DsRunnerText::Id::StatsPlayCount), entry->count);
    const auto& lastPlayed = entry->lastPlayed;
    if (!lastPlayed.IsValid() || length + 1 >= textSize)
        return;
    length += strlcpy(&text[length], DsRunnerText::Get(DsRunnerText::Id::StatsLastPlayed), textSize - length);
    if (length + 1 >= textSize)
        return;

    auto now = DsRunnerDateTime::Now();
    int days = now.IsValid() ? now.GetDayNumber() - lastPlayed.GetDayNumber() : -1;
    if (days == 0)
        strlcpy(&text[length], DsRunnerText::Get(DsRunnerText::Id::StatsToday), textSize - length);
    else if (days == 1)
        strlcpy(&text[length], DsRunnerText::Get(DsRunnerText::Id::StatsYesterday), textSize - length);
    else if (days > 1 && days <= DAYS_AGO_MAX)
        mini_snprintf(&text[length], textSize - length, DsRunnerText::Get(DsRunnerText::Id::StatsDaysAgo), days);
    else
    {
        // the year only for another year, or when the clock of the DS was set back
        bool withYear = days <= 0 || lastPlayed.year != now.year;
        if (DsRunnerText::IsEnglish())
        {
            const char* month = sMonthNames[lastPlayed.month - 1];
            if (withYear)
                mini_snprintf(&text[length], textSize - length, "on %s %d, %04d", month, lastPlayed.day, lastPlayed.year);
            else
                mini_snprintf(&text[length], textSize - length, "on %s %d", month, lastPlayed.day);
        }
        else if (withYear)
        {
            mini_snprintf(&text[length], textSize - length, "am %02d.%02d.%04d",
                lastPlayed.day, lastPlayed.month, lastPlayed.year);
        }
        else
            mini_snprintf(&text[length], textSize - length, "am %02d.%02d.", lastPlayed.day, lastPlayed.month);
    }
}

void DsRunnerFileInfoCardView::UpdateFileNameLine()
{
    // The bottom line shows the file name. Favorites get a star in front of it. On the console level,
    // where the folder name says nothing new, it tells which game START continues.
    _favoriteVersion = gDsRunnerLibrary.favorites.GetVersion();
    char text[sizeof(_fileName) + 160];
    const auto* appSettings = gDsRunnerLibrary.appSettings;
    const char* continuePath = gDsRunnerLibrary.continuePath;
    if (_isFolder && continuePath[0] != 0 && appSettings &&
        DsRunnerPaths::IsSameFolder(_folderPath, appSettings->libraryPath.GetString()))
    {
        // the game name without folder and extension
        const char* name = strrchr(continuePath, '/');
        name = name ? name + 1 : continuePath;
        char displayName[sizeof(_fileName)];
        DsRunnerNames::GetDisplayName(name, displayName, sizeof(displayName));
        mini_snprintf(text, sizeof(text), "%s%s)", DsRunnerText::Get(DsRunnerText::Id::ContinuePrefix), displayName);
    }
    else
    {
        text[0] = 0;
        if (!_isFolder && gDsRunnerLibrary.favorites.Contains(_folderPath, _fileName))
            strcpy(text, "\xE2\x98\x85 ");
        char consoleFolder[64];
        if (!_isFolder && gDsRunnerLibrary.searchResultsShown && appSettings &&
            DsRunnerPaths::GetConsoleFolderName(appSettings->libraryPath.GetString(), _folderPath,
                consoleFolder, sizeof(consoleFolder)))
        {
            // the search results come from all consoles: say which one
            const auto* console = DsRunnerConsoles::FindByFolderName(consoleFolder);
            strlcat(text, console ? DsRunnerText::Localize(console->fullName) : consoleFolder, sizeof(text));
            strlcat(text, " \xC2\xB7 ", sizeof(text));
        }
        if (_playStatsOnFileNameLine)
        {
            strlcat(text, _playStatsText, sizeof(text));
            strlcat(text, " \xC2\xB7 ", sizeof(text));
        }
        // the favorites and search entries have German internal names
        bool isEntry = DsRunnerFavoritesFolder::IsEntry(_fileName) || DsRunnerSearch::IsEntry(_fileName);
        strlcat(text, isEntry ? DsRunnerText::Localize(_fileName) : _fileName, sizeof(text));
    }

    if (_fileNameTaskQueue)
        _filenameLabelView->SetTextAsync(_fileNameTaskQueue, text);
    else
        _filenameLabelView->SetText(text);
}

void DsRunnerFileInfoCardView::RequestArt(const TCHAR* fileName)
{
    char path[256];
    bool hasPath = DsRunnerPaths::MirrorPath(ART_FOLDER, _folderPath, fileName, _isFolder, path, sizeof(path));
    // folders without an artwork of their own show the built-in image of their console
    char fallbackPath[128];
    bool hasFallback = _isFolder && gDsRunnerConsoleArt.IsAvailable() &&
        gDsRunnerConsoleArt.GetImagePath(fileName, DsRunnerConsoleArt::ImageType::Top, fallbackPath, sizeof(fallbackPath));
    _heroState->SetArt(hasPath ? path : nullptr, hasFallback ? fallbackPath : nullptr,
        DsRunnerConsoles::GetHeroColor(fileName), _ioTaskQueue);
}

bool DsRunnerFileInfoCardView::ShowGameCount(TaskQueueBase* taskQueue, const TCHAR* folderName)
{
    if (DsRunnerSearch::IsEntry(folderName))
    {
        // the search tile: the games of all consoles
        int total = gDsRunnerLibrary.gameCounts.GetTotal(_folderPath);
        if (total == DsRunnerGameCounts::Unknown)
            return false;
        char16_t text16[32];
        char text[32];
        DsRunnerText::FormatCount(text, sizeof(text), DsRunnerText::Id::SearchGamesOne,
            DsRunnerText::Id::SearchGamesMany, total);
        StringUtil::Copy(text16, text, sizeof(text16) / sizeof(text16[0]));
        SetSecondLineAsync(taskQueue, text16);
        return true;
    }

    // the favorites tile counts the favorites
    int count = DsRunnerFavoritesFolder::IsEntry(folderName)
        ? (int)gDsRunnerLibrary.favorites.GetCount()
        : gDsRunnerLibrary.gameCounts.Get(_folderPath, folderName);
    if (count == DsRunnerGameCounts::Unknown)
        return false;

    char text[24];
    if (count == 0)
        strcpy(text, DsRunnerText::Get(DsRunnerText::Id::NoGames));
    else
        DsRunnerText::FormatCount(text, sizeof(text), DsRunnerText::Id::GamesOne, DsRunnerText::Id::GamesMany, count);
    char16_t text16[24];
    int i = 0;
    for (; text[i] != 0; i++)
        text16[i] = (u8)text[i];
    text16[i] = 0;
    SetSecondLineAsync(taskQueue, text16);
    return true;
}

void DsRunnerFileInfoCardView::Update()
{
    if (_pendingCountFolder[0] != 0 && ShowGameCount(_textTaskQueue, _pendingCountFolder))
    {
        _pendingCountFolder[0] = 0;
    }
    if (_favoriteVersion != gDsRunnerLibrary.favorites.GetVersion() && _fileName[0] != 0)
    {
        // a favorite was added or removed (X): update the star
        UpdateFileNameLine();
    }

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

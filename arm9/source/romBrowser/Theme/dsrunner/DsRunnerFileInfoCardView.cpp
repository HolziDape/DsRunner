#include "common.h"
#include <string.h>
#include <algorithm>
#include "gui/GraphicsContext.h"
#include "services/settings/AppSettings.h"
#include "gui/VramContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "themes/dsrunner/DsRunnerConsoles.h"
#include "themes/dsrunner/DsRunnerConsoleArt.h"
#include "themes/dsrunner/DsRunnerHeroState.h"
#include "DsRunnerLibrary.h"
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
        SetFirstLineAsync(taskQueue, console->fullName, true);
        SetSecondLineAsync(taskQueue, u"", 0);
        SetThirdLineAsync(taskQueue, u"", 0);
    }
    else
    {
        BannerView::SetFileNameAsync(taskQueue, fileName, useAsTitle);
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
    UpdateFileNameLine();
}

void DsRunnerFileInfoCardView::UpdateFileNameLine()
{
    // The bottom line shows the file name. Favorites get a star in front of it. On the console level,
    // where the folder name says nothing new, it tells which game START continues.
    _favoriteVersion = gDsRunnerLibrary.favorites.GetVersion();
    char text[sizeof(_fileName) + 40];
    const auto* appSettings = gDsRunnerLibrary.appSettings;
    const char* continuePath = gDsRunnerLibrary.continuePath;
    if (_isFolder && continuePath[0] != 0 && appSettings &&
        DsRunnerPaths::IsSameFolder(_folderPath, appSettings->libraryPath.GetString()))
    {
        // the game name without folder and extension
        const char* name = strrchr(continuePath, '/');
        name = name ? name + 1 : continuePath;
        const char* extension = strrchr(name, '.');
        u32 nameLength = extension && extension != name ? extension - name : strlen(name);
        static constexpr char sPrefix[] = "START: Weiterspielen (";
        nameLength = std::min<u32>(nameLength, sizeof(text) - sizeof(sPrefix) - 1);
        memcpy(text, sPrefix, sizeof(sPrefix) - 1);
        memcpy(&text[sizeof(sPrefix) - 1], name, nameLength);
        strcpy(&text[sizeof(sPrefix) - 1 + nameLength], ")");
    }
    else if (!_isFolder && gDsRunnerLibrary.favorites.Contains(_folderPath, _fileName))
    {
        strcpy(text, "\xE2\x98\x85 ");
        strlcat(text, _fileName, sizeof(text));
    }
    else
    {
        strlcpy(text, _fileName, sizeof(text));
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
    // the favorites tile counts the favorites
    int count = DsRunnerFavoritesFolder::IsEntry(folderName)
        ? (int)gDsRunnerLibrary.favorites.GetCount()
        : gDsRunnerLibrary.gameCounts.Get(_folderPath, folderName);
    if (count == DsRunnerGameCounts::Unknown)
        return false;

    char text[24];
    if (count == 0)
        strcpy(text, "Keine Spiele");
    else if (count == 1)
        strcpy(text, "1 Spiel");
    else
    {
        // digits backwards, then reversed
        int length = 0;
        for (int value = count; value > 0; value /= 10)
            text[length++] = '0' + value % 10;
        for (int i = 0; i < length / 2; i++)
            std::swap(text[i], text[length - 1 - i]);
        strcpy(&text[length], " Spiele");
    }
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

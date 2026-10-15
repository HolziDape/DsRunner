#include "common.h"
#include <string.h>
#include <algorithm>
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "core/math/RgbMixer.h"
#include "gui/Gx.h"
#include "gui/GraphicsContext.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/PaletteManager.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/palette/GradientPalette.h"
#include "gui/font/nitroFont2.h"
#include "gui/input/InputProvider.h"
#include "themes/IFontRepository.h"
#include "themes/dsrunner/DsRunnerColorScheme.h"
#include "romBrowser/IRomBrowserController.h"
#include "DsRunnerShapes.h"
#include "DsRunnerSearchKeyboardView.h"

// header: title on the left, cancel button on the right
#define TITLE_X             12
#define TITLE_Y             6
#define CANCEL_X            178
#define CANCEL_Y            4
#define CANCEL_WIDTH        70
#define CANCEL_HEIGHT       20
// the query in its field, the status line (number of matches) below it
#define FIELD_X             8
#define FIELD_Y             28
#define FIELD_WIDTH         240
#define FIELD_HEIGHT        24
#define QUERY_X             16
#define QUERY_Y             32
#define STATUS_X            12
#define STATUS_Y            55
// keys: 4 rows of 10 columns
#define KEYS_X              4
#define KEYS_Y              74
#define KEY_STEP_X          25
#define KEY_STEP_Y          26
#define KEY_WIDTH           23
#define KEY_HEIGHT          23
#define KEY_RADIUS          5
#define KEY_ROWS            4
#define KEY_COLUMNS         10
#define HINT_X              8
#define HINT_Y              178

// the text sprite of a key: 32x16 (64x16 for the wide keys), the glyphs sit a bit low in it
#define CELL_HEIGHT         16
#define CELL_BYTES          (32 * CELL_HEIGHT / 2)
#define CELL_Y_OFFSET       1

// depth of the layers (raw 1.3.12 z values, smaller is closer), like the tiles (DsRunnerTileItemView)
#define Z_BODY              (-320)
#define Z_RING              (-200)
#define Z_CURSOR            (-330)
#define RING_WIDTH          2

using Rgb888 = Rgb<8, 8, 8>;

// key colors (letters, special keys, the search key, the selected key, the field): DsRunnerColors::Key*

// QWERTZ like a German keyboard, no umlauts: the search treats ä as a, ß as ss
const DsRunnerSearchKeyboardView::Key DsRunnerSearchKeyboardView::sKeys[] =
{
    { "1", 0, 0, 1, KeyAction::Character }, { "2", 0, 1, 1, KeyAction::Character },
    { "3", 0, 2, 1, KeyAction::Character }, { "4", 0, 3, 1, KeyAction::Character },
    { "5", 0, 4, 1, KeyAction::Character }, { "6", 0, 5, 1, KeyAction::Character },
    { "7", 0, 6, 1, KeyAction::Character }, { "8", 0, 7, 1, KeyAction::Character },
    { "9", 0, 8, 1, KeyAction::Character }, { "0", 0, 9, 1, KeyAction::Character },
    { "Q", 1, 0, 1, KeyAction::Character }, { "W", 1, 1, 1, KeyAction::Character },
    { "E", 1, 2, 1, KeyAction::Character }, { "R", 1, 3, 1, KeyAction::Character },
    { "T", 1, 4, 1, KeyAction::Character }, { "Z", 1, 5, 1, KeyAction::Character },
    { "U", 1, 6, 1, KeyAction::Character }, { "I", 1, 7, 1, KeyAction::Character },
    { "O", 1, 8, 1, KeyAction::Character }, { "P", 1, 9, 1, KeyAction::Character },
    { "A", 2, 0, 1, KeyAction::Character }, { "S", 2, 1, 1, KeyAction::Character },
    { "D", 2, 2, 1, KeyAction::Character }, { "F", 2, 3, 1, KeyAction::Character },
    { "G", 2, 4, 1, KeyAction::Character }, { "H", 2, 5, 1, KeyAction::Character },
    { "J", 2, 6, 1, KeyAction::Character }, { "K", 2, 7, 1, KeyAction::Character },
    { "L", 2, 8, 1, KeyAction::Character }, { "\xE2\x86\x90", 2, 9, 1, KeyAction::Backspace },
    { "Y", 3, 0, 1, KeyAction::Character }, { "X", 3, 1, 1, KeyAction::Character },
    { "C", 3, 2, 1, KeyAction::Character }, { "V", 3, 3, 1, KeyAction::Character },
    { "B", 3, 4, 1, KeyAction::Character }, { "N", 3, 5, 1, KeyAction::Character },
    { "M", 3, 6, 1, KeyAction::Character }, { "Leer", 3, 7, 1, KeyAction::Space },
    { "Suchen", 3, 8, 2, KeyAction::Search },
};

const u32 DsRunnerSearchKeyboardView::sKeyCount = sizeof(sKeys) / sizeof(sKeys[0]);

// the selection starts on the first letter
#define FIRST_LETTER_KEY    10

DsRunnerSearchKeyboardView::DsRunnerSearchKeyboardView(IRomBrowserController* romBrowserController,
    const IFontRepository* fontRepository)
    : _romBrowserController(romBrowserController)
    , _fontRepository(fontRepository)
    , _titleLabel(Label2DView::CreateShared(96, 16, 16, fontRepository->GetFont(FontType::Medium11)))
    , _queryLabel(Label2DView::CreateShared(224, 16, DsRunnerSearch::MaxQueryLength + 1,
        fontRepository->GetFont(FontType::Medium11)))
    , _statusLabel(Label2DView::CreateShared(232, 16, 96, fontRepository->GetFont(FontType::Regular10)))
    , _hintLabel(Label2DView::CreateShared(240, 16, 96, fontRepository->GetFont(FontType::Medium7_5)))
    , _selectedKey(FIRST_LETTER_KEY)
{
    static_assert(sizeof(sKeys) / sizeof(sKeys[0]) <= sizeof(_keyVramOffsets) / sizeof(_keyVramOffsets[0]));
    StringUtil::Copy(_query, romBrowserController->GetSearchQuery(), sizeof(_query));
    _titleLabel->SetText("Suche");
    _statusLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _hintLabel->SetText("B: L\xC3\xB6schen \xC2\xB7 X: Leerzeichen \xC2\xB7 START: Suchen \xC2\xB7 SELECT: Zur\xC3\xBC" "ck");
    UpdateQueryLabel();
    UpdateStatus();
}

void DsRunnerSearchKeyboardView::InitVram(const VramContext& vramContext)
{
    _titleLabel->InitVram(vramContext);
    _queryLabel->InitVram(vramContext);
    _statusLabel->InitVram(vramContext);
    _hintLabel->InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (!objVramManager)
        return;
    for (u32 i = 0; i < sKeyCount; i++)
    {
        u32 width = sKeys[i].span > 1 ? 64 : 32;
        _keyVramOffsets[i] = objVramManager->Alloc(width * CELL_HEIGHT / 2);
        // letters in the larger font, the words in the smaller one
        FontType fontType = sKeys[i].action == KeyAction::Character || sKeys[i].action == KeyAction::Backspace
            ? FontType::Medium11 : FontType::Medium7_5;
        if (sKeys[i].action == KeyAction::Search)
            fontType = FontType::Medium10;
        RenderText(sKeys[i].text, fontType, width, objVramManager->GetVramAddress(_keyVramOffsets[i]));
    }
    _cancelVramOffset = objVramManager->Alloc(64 * CELL_HEIGHT / 2);
    RenderText("Abbrechen", FontType::Regular10, 64, objVramManager->GetVramAddress(_cancelVramOffset));
    _hasVram = true;
}

void DsRunnerSearchKeyboardView::RenderText(const char* text, FontType fontType, u32 width, vu16* vram) const
{
    auto font = _fontRepository->GetFont(fontType);
    char16_t text16[16];
    StringUtil::Copy(text16, text, sizeof(text16) / sizeof(text16[0]));
    u32 textWidth, textHeight;
    nft2_measureString(font, text16, textWidth, textHeight);

    // 4 bpp, tiled in blocks of 32x16 pixels: exactly the sprites one after the other
    alignas(4) u8 tiles[64 * CELL_HEIGHT / 2];
    u32 bytes = width * CELL_HEIGHT / 2;
    memset(tiles, 0, bytes);
    nft2_string_render_params_t renderParams;
    renderParams.x = std::max(0, ((int)width - (int)textWidth) / 2);
    // the small font sits higher in its lines than the others
    renderParams.y = fontType == FontType::Medium7_5 ? 2 : 0;
    renderParams.width = width;
    renderParams.height = CELL_HEIGHT;
    renderParams.a5i3 = false;
    nft2_renderString(font, text16, tiles, width, &renderParams);
    const u32* source = (const u32*)tiles;
    vu32* destination = (vu32*)vram;
    for (u32 i = 0; i < bytes / 4; i++)
        destination[i] = source[i];
}

void DsRunnerSearchKeyboardView::UpdateQueryLabel()
{
    if (_query[0] == 0)
        _queryLabel->SetText("Name des Spiels");
    else
        _queryLabel->SetText(_query);
}

void DsRunnerSearchKeyboardView::UpdateStatus()
{
    _statusDirty = false;
    const auto& search = gDsRunnerSearch;
    char text[128];
    if (!search.IsReady())
    {
        strcpy(text, "Spiele werden gelesen\xE2\x80\xA6");
        _statusLabel->SetText(text);
        return;
    }

    char query[DsRunnerSearch::NormalizedSize];
    DsRunnerSearch::Normalize(_query, strlen(_query), query);
    if (query[0] == 0)
    {
        mini_snprintf(text, sizeof(text), "%d Spiele durchsuchen", search.GetGameCount());
        _statusLabel->SetText(text);
        return;
    }

    u16 groupCounts[DsRunnerSearch::MaxGroups];
    u32 count = search.Count(query, groupCounts);
    if (count == 0)
    {
        mini_snprintf(text, sizeof(text), "Keine Treffer f\xC3\xBCr \xE2\x80\x9E%s\xE2\x80\x9C", _query);
        _statusLabel->SetText(text);
        return;
    }

    // "12 Treffer: NDS 3 · GBA 5 · NES 4"
    int length;
    if (count > DsRunnerSearch::MaxResults)
        length = mini_snprintf(text, sizeof(text), "Mehr als %d Treffer (%d):", DsRunnerSearch::MaxResults, count);
    else
        length = mini_snprintf(text, sizeof(text), count == 1 ? "1 Treffer:" : "%d Treffer:", count);
    bool first = true;
    for (u32 group = 0; group < search.GetGroupCount() && length > 0 && length < (int)sizeof(text); group++)
    {
        if (groupCounts[group] == 0)
            continue;
        length += mini_snprintf(&text[length], sizeof(text) - length, first ? " %s %d" : " \xC2\xB7 %s %d",
            search.GetGroupLabel(group), groupCounts[group]);
        first = false;
    }
    _statusLabel->SetText(text);
}

void DsRunnerSearchKeyboardView::Update()
{
    _frameCounter++;
    bool isReady = gDsRunnerSearch.IsReady();
    if (isReady != _indexWasReady)
    {
        _indexWasReady = isReady;
        _statusDirty = true;
    }
    if (_statusDirty && !_showNoMatches)
        UpdateStatus();
    if (_searchPending && isReady)
    {
        _searchPending = false;
        Search();
    }

    _titleLabel->SetPosition(TITLE_X, TITLE_Y);
    _queryLabel->SetPosition(QUERY_X, QUERY_Y);
    _statusLabel->SetPosition(STATUS_X, STATUS_Y);
    _hintLabel->SetPosition(HINT_X, HINT_Y);
    _titleLabel->Update();
    _queryLabel->Update();
    _statusLabel->Update();
    _hintLabel->Update();
}

void DsRunnerSearchKeyboardView::VBlank()
{
    _titleLabel->VBlank();
    _queryLabel->VBlank();
    _statusLabel->VBlank();
    _hintLabel->VBlank();
}

Rectangle DsRunnerSearchKeyboardView::GetKeyBounds(const Key& key)
{
    return Rectangle(KEYS_X + key.column * KEY_STEP_X, KEYS_Y + key.row * KEY_STEP_Y,
        key.span * KEY_STEP_X - (KEY_STEP_X - KEY_WIDTH), KEY_HEIGHT);
}

void DsRunnerSearchKeyboardView::DrawKey(GraphicsContext& graphicsContext, const Key& key, u32 vramOffset,
    bool isSelected)
{
    auto bounds = GetKeyBounds(key);
    Rgb888 top, bottom, text;
    if (key.action == KeyAction::Search)
    {
        top = isSelected ? RgbMixer::Lerp(DsRunnerColors::Accent, DsRunnerColors::White, 1, 4) : DsRunnerColors::Accent;
        bottom = DsRunnerColors::AccentDim;
        text = DsRunnerColors::OnAccent;
    }
    else if (isSelected)
    {
        top = DsRunnerColors::KeySelectedTop;
        bottom = DsRunnerColors::KeySelectedBottom;
        text = DsRunnerColors::TextStrong;
    }
    else if (key.action == KeyAction::Character)
    {
        top = DsRunnerColors::KeyTop;
        bottom = DsRunnerColors::KeyBottom;
        text = DsRunnerColors::Text;
    }
    else
    {
        top = DsRunnerColors::KeySpecialTop;
        bottom = DsRunnerColors::KeySpecialBottom;
        text = DsRunnerColors::Text;
    }
    DsRunnerShapes::DrawRoundedRect(bounds.GetX(), bounds.GetY(), bounds.GetWidth(), bounds.GetHeight(), KEY_RADIUS, top, bottom, Z_BODY);
    if (isSelected)
    {
        // the ring fills the gap to the neighbor keys
        auto ring = key.action == KeyAction::Search ? DsRunnerColors::White : DsRunnerColors::Accent;
        DsRunnerShapes::DrawRoundedRect(bounds.GetX() - RING_WIDTH, bounds.GetY() - RING_WIDTH, bounds.GetWidth() + 2 * RING_WIDTH,
            bounds.GetHeight() + 2 * RING_WIDTH, KEY_RADIUS + RING_WIDTH, ring, ring, Z_RING);
    }

    if (!_hasVram)
        return;
    // the text: the font's 16 levels blend from the middle of the key to the text color
    u32 cellWidth = key.span > 1 ? 64 : 32;
    int x = bounds.GetX() + (bounds.GetWidth() - (int)cellWidth) / 2;
    int y = bounds.GetY() + (bounds.GetHeight() - CELL_HEIGHT) / 2 + CELL_Y_OFFSET;
    u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(RgbMixer::Lerp(top, bottom, 1, 2), text), y, y + CELL_HEIGHT);
    u32 cellCount = cellWidth / 32;
    auto oams = graphicsContext.GetOamManager().AllocOams(cellCount);
    for (u32 i = 0; i < cellCount; i++)
    {
        OamBuilder::OamWithSize<32, 16>(x + i * 32, y, (vramOffset + i * CELL_BYTES) >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(oams[i]);
    }
}

void DsRunnerSearchKeyboardView::Draw(GraphicsContext& graphicsContext)
{
    Gx::MtxIdentity();
    // no texture: the polygons show their vertex colors
    Gx::TexImageParam(0, false, false, false, false, GX_TEXSIZE_8, GX_TEXSIZE_8,
        GX_TEXFMT_NONE, false, GX_TEXGEN_NONE);
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_BOTH,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, graphicsContext.GetPolygonId());

    // header
    _titleLabel->SetBackgroundColor(DsRunnerColors::BgTop);
    _titleLabel->SetForegroundColor(DsRunnerColors::Text);
    _titleLabel->Draw(graphicsContext);
    bool cancelPressed = _penOnCancel;
    auto cancelColor = cancelPressed ? DsRunnerColors::KeySelectedTop : DsRunnerColors::Pill;
    DsRunnerShapes::DrawRoundedRect(CANCEL_X, CANCEL_Y, CANCEL_WIDTH, CANCEL_HEIGHT, CANCEL_HEIGHT / 2,
        cancelColor, cancelColor, Z_BODY);
    if (_hasVram)
    {
        int x = CANCEL_X + (CANCEL_WIDTH - 64) / 2;
        int y = CANCEL_Y + (CANCEL_HEIGHT - CELL_HEIGHT) / 2 + 1;
        u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(cancelColor, DsRunnerColors::Text), y, y + CELL_HEIGHT);
        auto oams = graphicsContext.GetOamManager().AllocOams(2);
        for (u32 i = 0; i < 2; i++)
        {
            OamBuilder::OamWithSize<32, 16>(x + i * 32, y, (_cancelVramOffset + i * CELL_BYTES) >> 7)
                .WithPalette16(paletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(oams[i]);
        }
    }

    // the query in its field, with a blinking cursor behind it
    DsRunnerShapes::DrawRoundedRect(FIELD_X - 1, FIELD_Y - 1, FIELD_WIDTH + 2, FIELD_HEIGHT + 2, 7,
        DsRunnerColors::Accent, DsRunnerColors::Accent, Z_RING);
    DsRunnerShapes::DrawRoundedRect(FIELD_X, FIELD_Y, FIELD_WIDTH, FIELD_HEIGHT, 6,
        DsRunnerColors::FieldTop, DsRunnerColors::FieldBottom, Z_BODY);
    _queryLabel->SetBackgroundColor(RgbMixer::Lerp(DsRunnerColors::FieldTop, DsRunnerColors::FieldBottom, 1, 2));
    _queryLabel->SetForegroundColor(_query[0] == 0 ? DsRunnerColors::TextDim : DsRunnerColors::TextStrong);
    _queryLabel->Draw(graphicsContext);
    if ((_frameCounter & 32) == 0)
    {
        int cursorX = QUERY_X + (_query[0] == 0 ? 0 : (int)_queryLabel->GetStringWidth() + 1);
        DsRunnerShapes::DrawRoundedRect(cursorX, FIELD_Y + 5, 2, FIELD_HEIGHT - 10, 0,
            DsRunnerColors::Accent, DsRunnerColors::Accent, Z_CURSOR);
    }

    _statusLabel->SetBackgroundColor(DsRunnerColors::FieldStatus);
    _statusLabel->SetForegroundColor(_showNoMatches ? DsRunnerColors::Favorite : DsRunnerColors::TextDim);
    _statusLabel->Draw(graphicsContext);

    for (u32 i = 0; i < sKeyCount; i++)
        DrawKey(graphicsContext, sKeys[i], _keyVramOffsets[i], (int)i == (_penKey >= 0 ? _penKey : _selectedKey));

    _hintLabel->SetBackgroundColor(DsRunnerColors::BgBottom);
    _hintLabel->SetForegroundColor(DsRunnerColors::TextDim);
    _hintLabel->Draw(graphicsContext);
}

int DsRunnerSearchKeyboardView::FindKey(int row, int column) const
{
    for (u32 i = 0; i < sKeyCount; i++)
    {
        if (sKeys[i].row == row && column >= sKeys[i].column && column < sKeys[i].column + sKeys[i].span)
            return i;
    }
    return -1;
}

int DsRunnerSearchKeyboardView::FindKeyAt(const Point& point) const
{
    // the gaps between the keys belong to the keys, so no touch gets lost
    int column = (point.x - KEYS_X + (KEY_STEP_X - KEY_WIDTH) / 2) / KEY_STEP_X;
    int row = (point.y - KEYS_Y + (KEY_STEP_Y - KEY_HEIGHT) / 2) / KEY_STEP_Y;
    if (point.x < KEYS_X - 2 || point.y < KEYS_Y - 2 || column < 0 || row < 0 ||
        column >= KEY_COLUMNS || row >= KEY_ROWS)
    {
        return -1;
    }
    return FindKey(row, column);
}

void DsRunnerSearchKeyboardView::MoveSelection(int dx, int dy)
{
    const auto& key = sKeys[_selectedKey];
    int row = key.row;
    int column = key.column;
    if (dx < 0)
        column = column > 0 ? column - 1 : KEY_COLUMNS - 1;
    else if (dx > 0)
        column = column + key.span < KEY_COLUMNS ? column + key.span : 0;
    else
        row = std::clamp(row + dy, 0, KEY_ROWS - 1);
    int index = FindKey(row, column);
    if (index >= 0)
        _selectedKey = index;
}

void DsRunnerSearchKeyboardView::Type(const Key& key)
{
    u32 length = strlen(_query);
    switch (key.action)
    {
        case KeyAction::Character:
        {
            if (length >= DsRunnerSearch::MaxQueryLength)
                return;
            char c = key.text[0];
            _query[length] = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
            _query[length + 1] = 0;
            break;
        }
        case KeyAction::Space:
        {
            // no space at the start and no two in a row
            if (length == 0 || length >= DsRunnerSearch::MaxQueryLength || _query[length - 1] == ' ')
                return;
            _query[length] = ' ';
            _query[length + 1] = 0;
            break;
        }
        case KeyAction::Backspace:
        {
            if (length == 0)
                return;
            _query[length - 1] = 0;
            break;
        }
        case KeyAction::Search:
        {
            Search();
            return;
        }
    }
    _showNoMatches = false;
    _statusDirty = true;
    UpdateQueryLabel();
}

void DsRunnerSearchKeyboardView::Search()
{
    char query[DsRunnerSearch::NormalizedSize];
    DsRunnerSearch::Normalize(_query, strlen(_query), query);
    if (query[0] == 0)
        return;
    if (!gDsRunnerSearch.IsReady())
    {
        // searched as soon as the games are read
        _searchPending = true;
        return;
    }
    if (gDsRunnerSearch.Count(query, nullptr) == 0)
    {
        // stay on the keyboard, the status line says so
        UpdateStatus();
        _showNoMatches = true;
        return;
    }
    // remove a space at the end, it would only show in the title
    u32 length = strlen(_query);
    if (length > 0 && _query[length - 1] == ' ')
        _query[length - 1] = 0;
    _romBrowserController->StartSearch(_query);
}

void DsRunnerSearchKeyboardView::Cancel()
{
    _searchPending = false;
    _romBrowserController->CloseSearch();
}

void DsRunnerSearchKeyboardView::HandleKeyboardInput(InputProvider& inputProvider)
{
    Point touchPoint;
    if (inputProvider.Triggered(InputKey::Touch) && inputProvider.GetCurrentTouchPoint(touchPoint))
    {
        _penKey = FindKeyAt(touchPoint);
        if (_penKey >= 0)
            _selectedKey = _penKey;
        _penOnCancel = Rectangle(CANCEL_X, CANCEL_Y, CANCEL_WIDTH, CANCEL_HEIGHT).Contains(touchPoint);
        _lastTouchPoint = touchPoint;
        return;
    }
    if (inputProvider.Released(InputKey::Touch))
    {
        // a key is typed when the pen leaves the screen on it
        if (_penKey >= 0 && FindKeyAt(_lastTouchPoint) == _penKey)
            Type(sKeys[_penKey]);
        else if (_penOnCancel && Rectangle(CANCEL_X, CANCEL_Y, CANCEL_WIDTH, CANCEL_HEIGHT).Contains(_lastTouchPoint))
            Cancel();
        _penKey = -1;
        _penOnCancel = false;
        return;
    }
    if (inputProvider.Current(InputKey::Touch))
    {
        if (inputProvider.GetCurrentTouchPoint(touchPoint))
            _lastTouchPoint = touchPoint;
        return;
    }

    if (inputProvider.Triggered(InputKey::DpadLeft))
        MoveSelection(-1, 0);
    else if (inputProvider.Triggered(InputKey::DpadRight))
        MoveSelection(1, 0);
    else if (inputProvider.Triggered(InputKey::DpadUp))
        MoveSelection(0, -1);
    else if (inputProvider.Triggered(InputKey::DpadDown))
        MoveSelection(0, 1);
    else if (inputProvider.Triggered(InputKey::A))
        Type(sKeys[_selectedKey]);
    else if (inputProvider.Triggered(InputKey::B))
    {
        // B deletes, with nothing left to delete it closes the keyboard
        if (_query[0] == 0)
            Cancel();
        else
            Type(sKeys[FindKey(2, 9)]);
    }
    else if (inputProvider.Triggered(InputKey::X))
        Type(sKeys[FindKey(3, 7)]);
    else if (inputProvider.Triggered(InputKey::Start))
        Search();
    else if (inputProvider.Triggered(InputKey::Select | InputKey::Y))
        Cancel();
}

bool DsRunnerSearchKeyboardView::HandleBrowserInput(const InputProvider& inputProvider,
    IRomBrowserController* romBrowserController)
{
    if (inputProvider.Triggered(InputKey::Select))
        return romBrowserController->OpenSearch();
    if (inputProvider.Triggered(InputKey::L | InputKey::R) && romBrowserController->IsCurrentFolderSearch())
        return romBrowserController->SwitchSearchFilter(inputProvider.Triggered(InputKey::L) ? -1 : 1);
    return false;
}

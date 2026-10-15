#pragma once
#include "core/math/Point.h"
#include "gui/views/View.h"
#include "gui/views/Label2DView.h"
#include "themes/FontType.h"
#include "DsRunnerSearch.h"

class IRomBrowserController;
class IFontRepository;
class InputProvider;

/// @brief On-screen keyboard of the search, shown instead of the rom browser on the bottom screen
///        (state RomBrowserState::SearchInput, owned by App). QWERTZ layout with digits, space,
///        backspace and "Suchen"; while typing a line shows how many games match, per console.
///        Works with the touchscreen and with the D-pad: A types the selected key, B deletes
///        (and closes the keyboard when the query is empty), X types a space, START searches,
///        SELECT or Y close it.
///        The keys are drawn with the 3D engine like the tiles, their text is one 32x16 sprite per key
///        (Suchen and the cancel button: 64x16). Palette rows per line: at most 4 in a key row (normal,
///        special, selected key, Suchen), 2 in the header, 1 elsewhere.
class DsRunnerSearchKeyboardView : public View
{
    SHARED_ONLY(DsRunnerSearchKeyboardView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

    /// @brief Handles keys and touch. The keyboard takes all input while it is shown.
    void HandleKeyboardInput(InputProvider& inputProvider);

    /// @brief Keys of the search while the rom browser has the input: SELECT opens the keyboard,
    ///        L and R switch the console in the search results.
    /// @return True when a key was used.
    static bool HandleBrowserInput(const InputProvider& inputProvider, IRomBrowserController* romBrowserController);

private:
    enum class KeyAction : u8
    {
        Character,
        Backspace,
        Space,
        Search
    };

    struct Key
    {
        const char* text;
        u8 row;
        u8 column;
        u8 span;
        KeyAction action;
    };

    static const Key sKeys[];
    static const u32 sKeyCount;

    IRomBrowserController* _romBrowserController;
    const IFontRepository* _fontRepository;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _queryLabel;
    SharedPtr<Label2DView> _statusLabel;
    SharedPtr<Label2DView> _hintLabel;

    char _query[DsRunnerSearch::MaxQueryLength + 1] = { 0 };
    // the query was changed or the index got ready: the status line is out of date
    bool _statusDirty = true;
    bool _indexWasReady = false;
    // START (or Suchen) was pressed before the games were read
    bool _searchPending = false;
    // "Keine Treffer" stays until the query changes
    bool _showNoMatches = false;
    // the selected key (D-pad) and the key held down with the pen
    int _selectedKey = 0;
    int _penKey = -1;
    bool _penOnCancel = false;
    Point _lastTouchPoint = Point(0, 0);
    u32 _frameCounter = 0;

    // text of the keys: 32x16 pixels per key, 64x16 for Suchen, then 64x16 for the cancel button
    u32 _keyVramOffsets[48] = { 0 };
    u32 _cancelVramOffset = 0;
    bool _hasVram = false;

    DsRunnerSearchKeyboardView(IRomBrowserController* romBrowserController, const IFontRepository* fontRepository);

    void RenderText(const char* text, FontType fontType, u32 width, vu16* vram) const;
    void UpdateQueryLabel();
    void UpdateStatus();
    void Type(const Key& key);
    void Search();
    void Cancel();
    void MoveSelection(int dx, int dy);
    int FindKey(int row, int column) const;
    int FindKeyAt(const Point& point) const;
    static Rectangle GetKeyBounds(const Key& key);
    void DrawKey(GraphicsContext& graphicsContext, const Key& key, u32 vramOffset, bool isSelected);
};

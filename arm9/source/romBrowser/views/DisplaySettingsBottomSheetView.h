#pragma once
#include <array>
#include "BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "IconButton2DView.h"
#include "../viewModels/DisplaySettingsViewModel.h"

class IRomBrowserController;
class MaterialColorScheme;
class IFontRepository;

class DisplaySettingsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(DisplaySettingsBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;

    void SetGraphics(const IconButton2DView::VramToken& iconButtonVramToken);

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(_layoutOptions[0]);
    }

protected:
    void Close() override;

private:
    /// DsRunner: number of language options (German, English)
    static constexpr u32 LanguageCount = 2;

    DisplaySettingsViewModel* _viewModel;

    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<IconButton2DView> _themeButton;
    // DsRunner: switches between the two pages ("Anzeige", "Spiel & Sprache"), also with L and R
    SharedPtr<IconButton2DView> _pageButton;
    SharedPtr<Label2DView> _layoutLabel;
    SharedPtr<Label2DView> _sortingLabel;
    // DsRunner: clock of the status bar on/off
    SharedPtr<Label2DView> _clockLabel;
    SharedPtr<IconButton2DView> _clockButton;
    // DsRunner: version, bottom right (see VERSION)
    SharedPtr<Label2DView> _versionLabel;

    // DsRunner: 3x2 tiles, 4x3 tiles, cover flow
    std::array<SharedPtr<IconButton2DView>, 3> _layoutOptions;
    std::array<SharedPtr<IconButton2DView>, 3> _sortOptions;

    // DsRunner: second page: return from games, its keys, language
    SharedPtr<IconButton2DView> _inGameResetButton;
    SharedPtr<Label2DView> _inGameResetLabel;
    std::array<SharedPtr<IconButton2DView>, DisplaySettingsViewModel::InGameResetKeyPresetCount + 1> _keyOptions;
    std::array<SharedPtr<Label2DView>, DisplaySettingsViewModel::InGameResetKeyPresetCount + 1> _keyLabels;
    std::array<SharedPtr<IconButton2DView>, LanguageCount> _languageOptions;
    std::array<SharedPtr<Label2DView>, LanguageCount> _languageLabels;

    const MaterialColorScheme* _materialColorScheme;

    // DsRunner: page shown (0 or 1), language of the texts shown
    u32 _page = 0;
    bool _textsEnglish = false;
    u32 _pageNextIconVramOffset = 0;
    u32 _pagePrevIconVramOffset = 0;
    u32 _radioOnIconVramOffset = 0;
    u32 _radioOffIconVramOffset = 0;

    SharedPtr<IconButton2DView> CreateLayoutOptionIconButton();
    SharedPtr<IconButton2DView> CreateSortOptionIconButton();
    SharedPtr<IconButton2DView> CreateToggleIconButton(IconButtonView::button_action_t action);

    DisplaySettingsBottomSheetView(DisplaySettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    void UpdateLabels();
    void UpdateTexts();
    void UpdateFirstPage(int x);
    void UpdateSecondPage(int x);
    void SetRadioState(IconButton2DView& button, bool selected) const;
    void SwitchPage(FocusManager* focusManager);
    SharedPtr<View> MoveFocusOnSecondPage(View* currentFocus, FocusMoveDirection direction);
    bool IsKeyOptionVisible(u32 index) const;

    u32 LoadIcon(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};

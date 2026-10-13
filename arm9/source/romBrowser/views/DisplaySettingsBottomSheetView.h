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
    DisplaySettingsViewModel* _viewModel;

    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<IconButton2DView> _themeButton;
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

    const MaterialColorScheme* _materialColorScheme;

    SharedPtr<IconButton2DView> CreateLayoutOptionIconButton();
    SharedPtr<IconButton2DView> CreateSortOptionIconButton();

    DisplaySettingsBottomSheetView(DisplaySettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    void UpdateLabels();

    u32 LoadIcon(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};

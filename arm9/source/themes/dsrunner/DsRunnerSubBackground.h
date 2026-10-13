#pragma once
#include <memory>
#include "themes/background/IThemeBackground.h"
#include "DsRunnerTopArt.h"

class MaterialColorScheme;
class DsRunnerHeroState;

/// @brief Top screen background: a vertical gradient in the color of the selected console,
///        fading into the flat dark band the file info text is drawn on. Above it the full screen
///        artwork of the selected console or game, when there is one.
class DsRunnerSubBackground : public IThemeBackground
{
public:
    DsRunnerSubBackground(const MaterialColorScheme* colorScheme, DsRunnerHeroState* heroState);

    void LoadResources(const ITheme& theme, const VramContext& vramContext) override;
    void Update() override;
    void VBlank() override;

private:
    const MaterialColorScheme* _colorScheme;
    DsRunnerHeroState* _heroState;
    // 48 KB of pixels: on the heap, not inside the theme object
    std::unique_ptr<DsRunnerTopArt> _art;
};

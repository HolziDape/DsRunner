#pragma once
#include "themes/background/IThemeBackground.h"

class MaterialColorScheme;
class DsRunnerHeroState;

/// @brief Top screen background: a vertical gradient in the color of the selected console,
///        fading into the flat dark band the file info text is drawn on.
class DsRunnerSubBackground : public IThemeBackground
{
public:
    DsRunnerSubBackground(const MaterialColorScheme* colorScheme, DsRunnerHeroState* heroState);

    void LoadResources(const ITheme& theme, const VramContext& vramContext) override;
    void VBlank() override;

private:
    const MaterialColorScheme* _colorScheme;
    DsRunnerHeroState* _heroState;
};

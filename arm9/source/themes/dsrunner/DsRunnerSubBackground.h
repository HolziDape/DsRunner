#pragma once
#include "themes/background/IThemeBackground.h"

class MaterialColorScheme;

class DsRunnerSubBackground : public IThemeBackground
{
public:
    explicit DsRunnerSubBackground(const MaterialColorScheme* colorScheme);

    void LoadResources(const ITheme& theme, const VramContext& vramContext) override;
    void VBlank() override;

private:
    const MaterialColorScheme* _colorScheme;
};

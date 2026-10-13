#pragma once
#include "themes/background/IThemeBackground.h"

class MaterialColorScheme;

class DsRunnerMainBackground : public IThemeBackground
{
public:
    explicit DsRunnerMainBackground(const MaterialColorScheme* colorScheme);

    void Draw(GraphicsContext& graphicsContext) override;
    void LoadResources(const ITheme& theme, const VramContext& vramContext) override;

private:
    const MaterialColorScheme* _colorScheme;
};

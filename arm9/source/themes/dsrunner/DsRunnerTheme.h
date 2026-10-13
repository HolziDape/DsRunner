#pragma once
#include <libtwl/gfx/gfxPalette.h>
#include "themes/Theme.h"
#include "DsRunnerColorScheme.h"
#include "romBrowser/Theme/Material/MaterialThemeFileIconFactory.h"
#include "romBrowser/Theme/dsrunner/DsRunnerRomBrowserViewFactory.h"
#include "DsRunnerMainBackground.h"
#include "DsRunnerSubBackground.h"
#include "themes/DefaultFontRepository.h"

class DsRunnerTheme : public Theme
{
public:
    DsRunnerTheme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode);

    const IFontRepository* GetFontRepository() const override
    {
        return &_fontRepository;
    }

    const IThemeFileIconFactory* GetThemeFileIconFactory() const override
    {
        return &_themeFileIconFactory;
    }

    const IRomBrowserViewFactory* GetRomBrowserViewFactory() const override
    {
        return &_romBrowserViewFactory;
    }

    std::unique_ptr<IThemeBackground> CreateRomBrowserTopBackground() const override
    {
        return std::make_unique<DsRunnerSubBackground>(&_materialColorScheme);
    }

    std::unique_ptr<IThemeBackground> CreateRomBrowserBottomBackground() const override
    {
        return std::make_unique<DsRunnerMainBackground>(&_materialColorScheme);
    }

    void LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext) override;

private:
    MaterialThemeFileIconFactory _themeFileIconFactory;
    DsRunnerRomBrowserViewFactory _romBrowserViewFactory;
    DefaultFontRepository _fontRepository;
};

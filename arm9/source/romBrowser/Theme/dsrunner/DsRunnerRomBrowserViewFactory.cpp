#include "common.h"
#include "DsRunnerRomBrowserViewFactory.h"

DsRunnerRomBrowserViewFactory::DsRunnerRomBrowserViewFactory(
    const MaterialColorScheme* colorScheme,
    const IFontRepository* fontRepository,
    DsRunnerHeroState* heroState)
    : _colorScheme(colorScheme)
    , _fontRepository(fontRepository)
    , _heroState(heroState) { }

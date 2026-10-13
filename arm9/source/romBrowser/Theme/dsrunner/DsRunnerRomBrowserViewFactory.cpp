#include "common.h"
#include "DsRunnerRomBrowserViewFactory.h"

DsRunnerRomBrowserViewFactory::DsRunnerRomBrowserViewFactory(
    const MaterialColorScheme* colorScheme,
    const IFontRepository* fontRepository)
    : _colorScheme(colorScheme)
    , _fontRepository(fontRepository) { }

#include "common.h"
#include <libtwl/mem/memVram.h>
#include "romBrowser/Theme/Material/CarouselRecyclerView.h"
#include "DsRunnerTheme.h"

DsRunnerTheme::DsRunnerTheme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode)
    : Theme(folderName, primaryColor, darkMode)
    , _themeFileIconFactory(&_materialColorScheme, &_fontRepository)
    , _romBrowserViewFactory(&_materialColorScheme, &_fontRepository)
{
    // Apply DsRunner dark colors to the color scheme
    DsRunnerColors::ApplyDarkScheme(_materialColorScheme);
}

void DsRunnerTheme::LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext)
{
    mem_setVramDMapping(MEM_VRAM_D_LCDC);
    mem_setVramEMapping(MEM_VRAM_E_LCDC);
    CarouselRecyclerView::UploadGraphics(mainVramContext);
    mem_setVramDMapping(MEM_VRAM_D_TEX_SLOT_0);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);
}

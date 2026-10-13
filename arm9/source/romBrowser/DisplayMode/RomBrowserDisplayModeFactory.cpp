#include "common.h"
#include "RomBrowserHorizontalIconGridDisplayMode.h"
#include "RomBrowserVerticalIconGridDisplayMode.h"
#include "RomBrowserBannerListDisplayMode.h"
#include "RomBrowserHorizontalCoverFlowDisplayMode.h"
#include "romBrowser/Theme/dsrunner/DsRunnerTileGridDisplayMode.h"
#include "RomBrowserDisplayModeFactory.h"

const RomBrowserDisplayMode* RomBrowserDisplayModeFactory::GetRomBrowserDisplayMode(
    RomBrowserLayout romBrowserDisplayMode) const
{
    switch (romBrowserDisplayMode)
    {
        case RomBrowserLayout::HorizontalIconGrid:
        {
            return &RomBrowserHorizontalIconGridDisplayMode::sInstance;
        }
        case RomBrowserLayout::VerticalIconGrid:
        {
            return &RomBrowserVerticalIconGridDisplayMode::sInstance;
        }
        case RomBrowserLayout::BannerList:
        {
            return &RomBrowserBannerListDisplayMode::sInstance;
        }
        case RomBrowserLayout::CoverFlow:
        {
            return &RomBrowserHorizontalCoverFlowDisplayMode::sInstance;
        }
        case RomBrowserLayout::TileGrid3x2:
        {
            return &DsRunnerTileGridDisplayMode::sInstance3x2;
        }
        case RomBrowserLayout::TileGrid4x3:
        {
            return &DsRunnerTileGridDisplayMode::sInstance4x3;
        }
        default:
        {
            return nullptr;
        }
    }
}
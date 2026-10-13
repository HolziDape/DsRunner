#pragma once
#include "RomBrowserLayout.h"
#include "RomBrowserSortMode.h"

class RomBrowserDisplaySettings
{
public:
    RomBrowserLayout layout = RomBrowserLayout::TileGrid3x2; // DsRunner: large tiles by default
    RomBrowserSortMode sortMode = RomBrowserSortMode::NameAscending;
};

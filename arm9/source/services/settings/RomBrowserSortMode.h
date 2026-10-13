#pragma once

enum class RomBrowserSortMode
{
    NameAscending,
    NameDescending,
    LastModified,
    // DsRunner: most recently played first (play history in /_dsrunner/recent.txt)
    LastPlayed
};
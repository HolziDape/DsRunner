#pragma once
#include <memory>
#include "ThemeInfo.h"

class ThemeInfoFactory
{
public:
    std::unique_ptr<ThemeInfo> CreateFromThemeFolder(const TCHAR* folderName) const;

    std::unique_ptr<ThemeInfo> CreateFallbackTheme() const
    {
        return std::make_unique<ThemeInfo>("dsrunner", ThemeType::DsRunner, "DsRunner", "", "", Rgb<8,8,8>(76, 194, 255), true);
    }
};

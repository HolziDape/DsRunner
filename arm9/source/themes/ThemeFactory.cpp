#include "common.h"
#include "material/MaterialTheme.h"
#include "custom/CustomTheme.h"
#include "dsrunner/DsRunnerTheme.h"
#include "ThemeFactory.h"

std::unique_ptr<ITheme> ThemeFactory::CreateFromThemeInfo(const ThemeInfo* themeInfo) const
{
    switch (themeInfo->GetType())
    {
        case ThemeType::Material:
        {
            return std::make_unique<MaterialTheme>(
                themeInfo->GetFolderName(),
                themeInfo->GetPrimaryColor(),
                themeInfo->GetIsDarkTheme());
        }
        case ThemeType::Custom:
        {
            return std::make_unique<CustomTheme>(
                themeInfo->GetFolderName(),
                themeInfo->GetPrimaryColor(),
                themeInfo->GetIsDarkTheme());
        }
        case ThemeType::DsRunner:
        {
            return std::make_unique<DsRunnerTheme>(
                themeInfo->GetFolderName(),
                themeInfo->GetPrimaryColor(),
                themeInfo->GetIsDarkTheme());
        }
        default:
        {
            return nullptr;
        }
    }
}
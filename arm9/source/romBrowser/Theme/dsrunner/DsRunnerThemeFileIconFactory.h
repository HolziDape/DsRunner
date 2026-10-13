#pragma once
#include "romBrowser/Theme/IThemeFileIconFactory.h"
#include "romBrowser/Theme/Material/MaterialThemeFileIconFactory.h"
#include "DsRunnerFolderIcon.h"
#include "themes/dsrunner/DsRunnerConsoles.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief File icons of the DsRunner theme. Folders get console badges, everything else
///        uses the Material icons (NDS banners are drawn from the game itself anyway).
class DsRunnerThemeFileIconFactory : public IThemeFileIconFactory
{
public:
    DsRunnerThemeFileIconFactory(const MaterialColorScheme* colorScheme, const IFontRepository* fontRepository)
        : _materialFactory(colorScheme, fontRepository), _fontRepository(fontRepository) { }

    std::unique_ptr<FileIcon> CreateFolderIcon(const TCHAR* name) const override
    {
        return std::make_unique<DsRunnerFolderIcon>(name, _fontRepository);
    }

    std::unique_ptr<FileIcon> CreateGenericFileIcon(const TCHAR* name) const override
    {
        // files of a known console (e.g. .gba without a banner) get the badge of that console
        if (const auto* console = DsRunnerConsoles::FindByFileName(name))
        {
            return std::make_unique<DsRunnerFolderIcon>(console->label, console->tile, _fontRepository);
        }
        return _materialFactory.CreateGenericFileIcon(name);
    }

    std::unique_ptr<FileIcon> CreateNdsFileIcon(const TCHAR* name) const override
    {
        return _materialFactory.CreateNdsFileIcon(name);
    }

    std::unique_ptr<FileIcon> CreateThemeFileIcon() const override
    {
        return _materialFactory.CreateThemeFileIcon();
    }

    bool UseFileNameForInfoIcon() const override { return true; }

private:
    MaterialThemeFileIconFactory _materialFactory;
    const IFontRepository* _fontRepository;
};

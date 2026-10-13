#pragma once

class FileIcon;

class IThemeFileIconFactory
{
public:
    virtual ~IThemeFileIconFactory() = 0;
    virtual std::unique_ptr<FileIcon> CreateFolderIcon(const TCHAR* name) const = 0;
    virtual std::unique_ptr<FileIcon> CreateGenericFileIcon(const TCHAR* name) const = 0;
    virtual std::unique_ptr<FileIcon> CreateNdsFileIcon(const TCHAR* name) const = 0;
    virtual std::unique_ptr<FileIcon> CreateThemeFileIcon() const = 0;

    /// @brief Whether the icon on the top screen is created with the real file name (so name
    ///        dependent icons like console badges look the same as in the list), instead of an empty name.
    virtual bool UseFileNameForInfoIcon() const { return false; }
};

inline IThemeFileIconFactory::~IThemeFileIconFactory() { }

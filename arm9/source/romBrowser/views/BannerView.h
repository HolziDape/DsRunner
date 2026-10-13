#pragma once
#include "gui/views/ViewContainer.h"
#include "../FileType/FileIcon.h"

class TaskQueueBase;

class BannerView : public ViewContainer
{
public:
    void InitVram(const VramContext& vramContext) override;

    void SetFileName(const TCHAR* fileName, bool useAsTitle)
    {
        SetFileNameAsync(nullptr, fileName, useAsTitle);
    }

    virtual void SetFileNameAsync(TaskQueueBase* taskQueue, const TCHAR* fileName, bool useAsTitle);

    /// @brief DsRunner: tells the view where the item of the next SetFileNameAsync call is
    ///        (for the artwork and game counts of the DsRunner theme).
    /// @param folderPath The folder that contains the item.
    /// @param isFolder True when the item is a folder.
    /// @param ioTaskQueue The queue for file access.
    virtual void SetItemContext(const TCHAR* folderPath, bool isFolder, TaskQueueBase* ioTaskQueue) { }

    void SetGameTitle(const char16_t* gameTitle)
    {
        SetGameTitleAsync(nullptr, gameTitle);
    }

    void SetGameTitleAsync(TaskQueueBase* taskQueue, const char16_t* gameTitle);

    void SetIcon(std::unique_ptr<FileIcon> icon)
    {
        _icon = std::move(icon);
        if (_icon)
        {
            _icon->SetVramAddress(_iconVram, _iconVramOffset);
        }
    }

    void UploadIconGraphics() const
    {
        if (_icon)
        {
            _icon->UploadGraphics();
        }
    }

protected:
    std::unique_ptr<FileIcon> _icon = nullptr;
    vu16* _iconVram = nullptr;
    u32 _iconVramOffset = 0;
    u32 _lines = 3;

    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char* firstLine, bool ellipsis) = 0;
    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, bool ellipsis) = 0;
    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, u32 length, bool ellipsis) = 0;
    virtual void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine) = 0;
    virtual void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine, u32 length) = 0;
    virtual void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine) = 0;
    virtual void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine, u32 length) = 0;
};

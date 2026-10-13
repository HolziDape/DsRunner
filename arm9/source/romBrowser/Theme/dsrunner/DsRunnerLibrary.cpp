#include "common.h"
#include <string.h>
#include "fat/File.h"
#include "fat/Directory.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/FileType/FileType.h"
#include "romBrowser/FileType/IFileTypeProvider.h"
#include "DsRunnerLibrary.h"

#define DSRUNNER_FOLDER         "/_dsrunner"
#define PLAY_HISTORY_PATH       "/_dsrunner/recent.txt"
// larger history files are cut off (100 entries of 256 bytes at most)
#define PLAY_HISTORY_MAX_SIZE   (DsRunnerPlayHistory::MaxEntries * 256)

DsRunnerLibrary gDsRunnerLibrary;

const char* DsRunnerPaths::SkipDrive(const char* path)
{
    const char* colon = strchr(path, ':');
    return colon ? colon + 1 : path;
}

// length of a folder path without drive and trailing slashes ("/" becomes "")
static u32 folderLength(const char* path)
{
    u32 length = strlen(path);
    while (length > 0 && path[length - 1] == '/')
        length--;
    return length;
}

bool DsRunnerPaths::IsSameFolder(const char* a, const char* b)
{
    a = SkipDrive(a);
    b = SkipDrive(b);
    u32 lengthA = folderLength(a);
    u32 lengthB = folderLength(b);
    return lengthA == lengthB && strncasecmp(a, b, lengthA) == 0;
}

void DsRunnerPlayHistory::Load()
{
    if (_loaded)
        return;
    _loaded = true;

    auto file = std::make_unique<File>();
    if (file->Open(PLAY_HISTORY_PATH, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return;

    u32 size = std::min<u32>(file->GetSize(), PLAY_HISTORY_MAX_SIZE);
    auto text = std::make_unique<char[]>(size + 1);
    u32 bytesRead = 0;
    if (file->Read(text.get(), size, bytesRead) != FR_OK)
        return;
    text[bytesRead] = 0;

    char* line = text.get();
    while (*line != 0 && _count < MaxEntries)
    {
        char* end = line;
        while (*end != 0 && *end != '\n' && *end != '\r')
            end++;
        char next = *end;
        *end = 0;
        // paths may start with a drive ("fat:/Games/..."), the entries are kept without it
        const char* path = DsRunnerPaths::SkipDrive(line);
        if (path[0] == '/')
        {
            u32 length = end - path;
            _entries[_count] = std::make_unique<char[]>(length + 1);
            memcpy(_entries[_count].get(), path, length + 1);
            _count++;
        }
        line = next != 0 ? end + 1 : end;
    }
    LOG_DEBUG("Play history: %d entries\n", _count);
}

void DsRunnerPlayHistory::Add(const char* path)
{
    Load();
    path = DsRunnerPaths::SkipDrive(path);

    // take the entry out if it is already in the list, otherwise drop the oldest one
    u32 index = 0;
    while (index < _count && strcasecmp(_entries[index].get(), path) != 0)
        index++;
    std::unique_ptr<char[]> entry;
    if (index < _count)
    {
        entry = std::move(_entries[index]);
    }
    else
    {
        u32 length = strlen(path);
        entry = std::make_unique<char[]>(length + 1);
        memcpy(entry.get(), path, length + 1);
        if (_count < MaxEntries)
            _count++;
        index = _count - 1;
    }
    for (u32 i = index; i > 0; i--)
        _entries[i] = std::move(_entries[i - 1]);
    _entries[0] = std::move(entry);

    Save();
}

void DsRunnerPlayHistory::Save() const
{
    f_mkdir(DSRUNNER_FOLDER);
    auto file = std::make_unique<File>();
    if (file->Open(PLAY_HISTORY_PATH, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't write the play history\n");
        return;
    }
    for (u32 i = 0; i < _count; i++)
    {
        u32 bytesWritten;
        file->Write(_entries[i].get(), strlen(_entries[i].get()), bytesWritten);
        file->Write("\n", 1, bytesWritten);
    }
}

int DsRunnerPlayHistory::GetRank(const char* folderPath, const char* fileName) const
{
    folderPath = DsRunnerPaths::SkipDrive(folderPath);
    u32 length = folderLength(folderPath);
    for (u32 i = 0; i < _count; i++)
    {
        const char* entry = _entries[i].get();
        if (strncasecmp(entry, folderPath, length) == 0 && entry[length] == '/' &&
            strcasecmp(entry + length + 1, fileName) == 0)
        {
            return i;
        }
    }
    return -1;
}

bool DsRunnerGameCounts::NeedsCounting(const char* libraryPath) const
{
    if (_libraryPath[0] == 0 || !DsRunnerPaths::IsSameFolder(_libraryPath, libraryPath))
        return true;
    // an earlier count was canceled (e.g. by leaving the launcher)
    for (u32 i = 0; i < _entryCount; i++)
    {
        if (_entries[i].count == Unknown)
            return true;
    }
    return false;
}

void DsRunnerGameCounts::SetFolders(const char* libraryPath, const SdFolder& folder)
{
    strlcpy(_libraryPath, DsRunnerPaths::SkipDrive(libraryPath), sizeof(_libraryPath));
    if (_libraryPath[0] == 0)
        strlcpy(_libraryPath, "/", sizeof(_libraryPath));
    _entryCount = 0;
    for (int i = 0; i < folder.GetFileCount() && _entryCount < MaxFolders; i++)
    {
        const auto* file = folder.GetFiles()[i];
        if (file->GetFileType()->GetClassification() != FileTypeClassification::Folder ||
            strlen(file->GetFileName()) >= sizeof(Entry::name))
        {
            continue;
        }
        auto& entry = _entries[_entryCount++];
        strcpy(entry.name, file->GetFileName());
        entry.count = Unknown;
    }
}

void DsRunnerGameCounts::Count(const IFileTypeProvider& fileTypeProvider, const vu8& cancelRequested)
{
    auto fileInfo = std::make_unique<FILINFO>();
    auto path = std::make_unique<char[]>(512);
    for (u32 i = 0; i < _entryCount && !cancelRequested; i++)
    {
        auto& entry = _entries[i];
        if (entry.count != Unknown)
            continue;

        u32 length = folderLength(_libraryPath);
        memcpy(path.get(), _libraryPath, length);
        path[length] = '/';
        strcpy(&path[length + 1], entry.name);

        Directory directory;
        if (directory.Open(path.get()) != FR_OK)
        {
            entry.count = 0;
            continue;
        }
        // the same files the rom browser shows: known types, no folders, nothing hidden
        int count = 0;
        while (!cancelRequested && directory.Read(fileInfo.get()) == FR_OK && fileInfo->fname[0] != 0)
        {
            if ((fileInfo->fattrib & (AM_DIR | AM_HID)) || fileInfo->fname[0] == '.')
                continue;
            auto fileType = fileTypeProvider.GetFileType(fileInfo->fname);
            if (fileType && fileType->GetClassification() != FileTypeClassification::Unknown)
                count++;
        }
        if (!cancelRequested)
            entry.count = count;
    }
}

int DsRunnerGameCounts::Get(const char* folderPath, const char* folderName) const
{
    if (_libraryPath[0] == 0 || !DsRunnerPaths::IsSameFolder(folderPath, _libraryPath))
        return Unknown;
    for (u32 i = 0; i < _entryCount; i++)
    {
        if (!strcasecmp(_entries[i].name, folderName))
            return _entries[i].count;
    }
    return Unknown;
}

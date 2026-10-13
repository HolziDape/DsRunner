#include "common.h"
#include <string.h>
#include "fat/File.h"
#include "fat/Directory.h"
#include "core/task/TaskQueue.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/FileType/FileType.h"
#include "romBrowser/FileType/IFileTypeProvider.h"
#include "romBrowser/FileType/Folder/FolderFileType.h"
#include "DsRunnerLibrary.h"

#define DSRUNNER_FOLDER         "/_dsrunner"
#define PLAY_HISTORY_PATH       "/_dsrunner/recent.txt"
#define FAVORITES_PATH          "/_dsrunner/favorites.txt"
// longer lines are not valid paths for the launcher anyway
#define MAX_LINE_LENGTH         256

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

bool DsRunnerPaths::Combine(const char* folderPath, const char* fileName, char* result, u32 resultSize)
{
    folderPath = SkipDrive(folderPath);
    u32 length = folderLength(folderPath);
    u32 nameLength = strlen(fileName);
    if (length + 1 + nameLength + 1 > resultSize)
        return false;
    memcpy(result, folderPath, length);
    result[length] = '/';
    memcpy(&result[length + 1], fileName, nameLength + 1);
    return true;
}

bool DsRunnerPaths::MirrorPath(const char* root, const char* folderPath, const char* name, bool isFolder,
    char* result, u32 resultSize)
{
    folderPath = SkipDrive(folderPath);
    u32 rootLength = strlen(root);
    u32 length = folderLength(folderPath);
    u32 nameLength = strlen(name);
    const char* extension = isFolder ? nullptr : strrchr(name, '.');
    if (extension && extension != name)
        nameLength = extension - name;
    if (rootLength + length + 1 + nameLength + 4 >= resultSize)
        return false;
    char* p = result;
    memcpy(p, root, rootLength);
    p += rootLength;
    memcpy(p, folderPath, length);
    p += length;
    *p++ = '/';
    memcpy(p, name, nameLength);
    p += nameLength;
    strcpy(p, ".bmp");
    return true;
}

// Returns true when the full path \p entry is the file \p fileName in the folder \p folderPath,
// where \p folderPathLength is the folderLength() of the folder path (without drive).
static bool isFileInFolder(const char* entry, const char* folderPath, u32 folderPathLength, const char* fileName)
{
    return strncasecmp(entry, folderPath, folderPathLength) == 0 && entry[folderPathLength] == '/' &&
        strcasecmp(entry + folderPathLength + 1, fileName) == 0;
}

static std::unique_ptr<char[]> copyString(const char* string, u32 length)
{
    auto copy = std::make_unique<char[]>(length + 1);
    memcpy(copy.get(), string, length);
    copy[length] = 0;
    return copy;
}

// Reads a text file with one path per line into \p entries. Paths may start with a drive ("fat:/Games/..."),
// the entries are kept without it. Lines that are no absolute path are skipped.
static u32 readPathList(const char* filePath, std::unique_ptr<char[]>* entries, u32 maxEntries)
{
    auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return 0;

    // larger files are cut off
    u32 size = std::min<u32>(file->GetSize(), maxEntries * MAX_LINE_LENGTH);
    auto text = std::make_unique<char[]>(size + 1);
    u32 bytesRead = 0;
    if (file->Read(text.get(), size, bytesRead) != FR_OK)
        return 0;
    text[bytesRead] = 0;

    u32 count = 0;
    char* line = text.get();
    while (*line != 0 && count < maxEntries)
    {
        char* end = line;
        while (*end != 0 && *end != '\n' && *end != '\r')
            end++;
        char next = *end;
        *end = 0;
        const char* path = DsRunnerPaths::SkipDrive(line);
        if (path[0] == '/')
            entries[count++] = copyString(path, end - path);
        line = next != 0 ? end + 1 : end;
    }
    return count;
}

// Writes \p length bytes of \p text to \p filePath, creating the DsRunner folder if needed.
static void writeTextFile(const char* filePath, const char* text, u32 length)
{
    f_mkdir(DSRUNNER_FOLDER);
    auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't write %s\n", filePath);
        return;
    }
    u32 bytesWritten;
    file->Write(text, length, bytesWritten);
}

void DsRunnerPlayHistory::Load()
{
    if (_loaded)
        return;
    _loaded = true;
    _count = readPathList(PLAY_HISTORY_PATH, _entries, MaxEntries);
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
        entry = copyString(path, strlen(path));
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
        if (isFileInFolder(_entries[i].get(), folderPath, length, fileName))
            return i;
    }
    return -1;
}

void DsRunnerFavorites::Load()
{
    if (_loaded)
        return;
    _loaded = true;
    _count = readPathList(FAVORITES_PATH, _entries, MaxEntries);
    _version++;
    LOG_DEBUG("Favorites: %d entries\n", _count);
}

int DsRunnerFavorites::Find(const char* folderPath, const char* fileName) const
{
    folderPath = DsRunnerPaths::SkipDrive(folderPath);
    u32 length = folderLength(folderPath);
    for (u32 i = 0; i < _count; i++)
    {
        if (isFileInFolder(_entries[i].get(), folderPath, length, fileName))
            return i;
    }
    return -1;
}

bool DsRunnerFavorites::Toggle(const char* folderPath, const char* fileName, TaskQueueBase* ioTaskQueue)
{
    bool isFavorite;
    int index = Find(folderPath, fileName);
    if (index >= 0)
    {
        for (u32 i = index; i + 1 < _count; i++)
            _entries[i] = std::move(_entries[i + 1]);
        _entries[--_count].reset();
        isFavorite = false;
    }
    else
    {
        char path[MAX_LINE_LENGTH];
        if (_count >= MaxEntries || !DsRunnerPaths::Combine(folderPath, fileName, path, sizeof(path)))
            return false;
        _entries[_count++] = copyString(path, strlen(path));
        isFavorite = true;
    }
    _version++;

    // The io thread must not touch the entries while the main thread changes them, so it gets its own
    // copy of the file content. Tasks run in order, so the last toggle wins.
    u32 length = 0;
    for (u32 i = 0; i < _count; i++)
        length += strlen(_entries[i].get()) + 1;
    auto text = std::make_unique<char[]>(length + 1);
    char* p = text.get();
    for (u32 i = 0; i < _count; i++)
    {
        u32 entryLength = strlen(_entries[i].get());
        memcpy(p, _entries[i].get(), entryLength);
        p += entryLength;
        *p++ = '\n';
    }
    *p = 0;
    ioTaskQueue->Enqueue([text = std::move(text), length] (const vu8& cancelRequested)
    {
        writeTextFile(FAVORITES_PATH, text.get(), length);
        return TaskResult<void>::Completed();
    });
    return isFavorite;
}

bool DsRunnerFavoritesFolder::IsEntry(const char* name)
{
    return name && strcmp(name, EntryName) == 0;
}

void DsRunnerFavoritesFolder::AddEntry(SdFolder& libraryRoot, const DsRunnerFavorites& favorites)
{
    if (favorites.GetCount() == 0)
        return;
    // a folder entry that exists nowhere on the card: an empty file reference
    DIR directory;
    FILINFO fileInfo;
    memset(&directory, 0, sizeof(directory));
    memset(&fileInfo, 0, sizeof(fileInfo));
    libraryRoot.AddFile(new FileInfo(EntryName, &FolderFileType::sInstance,
        FastFileRef(&directory, &fileInfo), AM_DIR));
}

void DsRunnerFavoritesFolder::MoveEntryToFront(const FileInfo** files, u32 count)
{
    for (u32 i = 1; i < count; i++)
    {
        if (IsEntry(files[i]->GetFileName()))
        {
            const FileInfo* entry = files[i];
            for (u32 j = i; j > 0; j--)
                files[j] = files[j - 1];
            files[0] = entry;
            return;
        }
    }
}

std::unique_ptr<DsRunnerFavoritesFolder> DsRunnerFavoritesFolder::Create(const DsRunnerFavorites& favorites,
    const IFileTypeProvider& fileTypeProvider, std::unique_ptr<SdFolder>& folder)
{
    u32 count = favorites.GetCount();
    auto result = std::make_unique<DsRunnerFavoritesFolder>();
    result->_folders = std::make_unique<std::unique_ptr<char[]>[]>(count);
    result->_items = std::make_unique<Item[]>(count);
    // the sd folder takes the array over and frees it with free()
    auto files = (FileInfo**)malloc(sizeof(FileInfo*) * (count > 0 ? count : 1));
    u32 fileCount = 0;
    u32 folderCount = 0;

    // each folder is read once and matched against all favorites in it
    auto done = std::make_unique<bool[]>(count);
    auto sdFileInfo = std::make_unique<FILINFO>();
    for (u32 i = 0; i < count; i++)
    {
        if (done[i])
            continue;
        const char* path = favorites.Get(i);
        u32 folderPathLength = strrchr(path, '/') - path;
        u32 folderIndex = folderCount++;
        result->_folders[folderIndex] = copyString(path, folderPathLength);
        const char* folderPath = folderPathLength == 0 ? "/" : result->_folders[folderIndex].get();

        Directory directory;
        bool isOpen = directory.Open(folderPath) == FR_OK;
        while (isOpen && directory.Read(sdFileInfo.get()) == FR_OK && sdFileInfo->fname[0] != 0)
        {
            if (sdFileInfo->fattrib & AM_DIR)
                continue;
            for (u32 j = i; j < count; j++)
            {
                if (!done[j] && isFileInFolder(favorites.Get(j), path, folderPathLength, sdFileInfo->fname))
                {
                    done[j] = true;
                    auto file = new FileInfo(sdFileInfo->fname, fileTypeProvider.GetFileType(sdFileInfo->fname),
                        FastFileRef(directory.GetFatFsDirectory(), sdFileInfo.get()), sdFileInfo->fattrib);
                    files[fileCount] = file;
                    result->_items[fileCount] = { file, folderIndex };
                    fileCount++;
                    break;
                }
            }
        }

        // favorites of this folder that weren't found don't exist anymore
        for (u32 j = i; j < count; j++)
        {
            const char* other = favorites.Get(j);
            if (!done[j] && strncasecmp(other, path, folderPathLength) == 0 &&
                other[folderPathLength] == '/' && strchr(other + folderPathLength + 1, '/') == nullptr)
            {
                done[j] = true;
            }
        }
    }
    result->_itemCount = fileCount;
    LOG_DEBUG("Favorites folder: %d of %d files found\n", fileCount, count);
    folder = std::make_unique<SdFolder>(files, fileCount);
    return result;
}

const char* DsRunnerFavoritesFolder::GetFolderPath(const FileInfo* file) const
{
    for (u32 i = 0; i < _itemCount; i++)
    {
        if (_items[i].file == file)
        {
            const char* folder = _folders[_items[i].folderIndex].get();
            return folder[0] != 0 ? folder : "/";
        }
    }
    return nullptr;
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
        // the favorites entry is no real folder, its count comes from the favorites
        if (file->GetFileType()->GetClassification() != FileTypeClassification::Folder ||
            strlen(file->GetFileName()) >= sizeof(Entry::name) ||
            DsRunnerFavoritesFolder::IsEntry(file->GetFileName()))
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

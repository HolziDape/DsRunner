#include "common.h"
#include <string.h>
#include "fat/File.h"
#include "fat/Directory.h"
#include "core/task/TaskQueue.h"
#include "core/mini-printf.h"
#include "rtcIpc.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/FileType/FileType.h"
#include "romBrowser/FileType/IFileTypeProvider.h"
#include "romBrowser/FileType/Folder/FolderFileType.h"
#include "DsRunnerSearch.h"
#include "DsRunnerLibrary.h"

#define DSRUNNER_FOLDER         "/_dsrunner"
#define PLAY_HISTORY_PATH       "/_dsrunner/recent.txt"
#define FAVORITES_PATH          "/_dsrunner/favorites.txt"
#define PLAY_STATS_PATH         "/_dsrunner/stats.txt"
// "|<count>|YYYY-MM-DD HH:MM" after the path of a statistics line
#define PLAY_STATS_SUFFIX_SIZE  24
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

bool DsRunnerPaths::GetConsoleFolderName(const char* libraryPath, const char* folderPath, char* result,
    u32 resultSize)
{
    libraryPath = SkipDrive(libraryPath);
    folderPath = SkipDrive(folderPath);
    u32 libraryLength = folderLength(libraryPath);
    if (strncasecmp(folderPath, libraryPath, libraryLength) != 0 || folderPath[libraryLength] != '/')
        return false;
    const char* name = folderPath + libraryLength + 1;
    u32 length = 0;
    while (name[length] != 0 && name[length] != '/')
        length++;
    if (length == 0 || length >= resultSize)
        return false;
    memcpy(result, name, length);
    result[length] = 0;
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

static int fromBcd(u8 value)
{
    return (value >> 4) * 10 + (value & 0x0F);
}

DsRunnerDateTime DsRunnerDateTime::Now()
{
    rtc_datetime_t rtc;
    rtc_readDateTime(&rtc);
    DsRunnerDateTime result;
    result.year = 2000 + fromBcd(rtc.date.year);
    result.month = fromBcd(rtc.date.month);
    result.day = fromBcd(rtc.date.monthDay);
    result.hour = fromBcd(rtc.time.hour);
    result.minute = fromBcd(rtc.time.minute);
    return result;
}

int DsRunnerDateTime::GetDayNumber() const
{
    // days from the civil date (gregorian calendar), see http://howardhinnant.github.io/date_algorithms.html
    int y = year - (month <= 2 ? 1 : 0);
    int era = y / 400;
    int yearOfEra = y - era * 400;
    int dayOfYear = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + dayOfEra - 719468;
}

// Reads a number of at most \p maxDigits digits at \p p, -1 when there is none.
static int parseNumber(const char*& p, u32 maxDigits)
{
    int value = -1;
    for (u32 i = 0; i < maxDigits && *p >= '0' && *p <= '9'; i++)
        value = (value < 0 ? 0 : value * 10) + (*p++ - '0');
    return value;
}

// Parses "YYYY-MM-DD HH:MM" (the time may be missing), an unknown date when it isn't one.
static DsRunnerDateTime parseDateTime(const char* p)
{
    DsRunnerDateTime result;
    int year = parseNumber(p, 4);
    if (*p++ != '-')
        return result;
    int month = parseNumber(p, 2);
    if (*p++ != '-')
        return result;
    int day = parseNumber(p, 2);
    int hour = 0;
    int minute = 0;
    if (*p == ' ')
    {
        p++;
        hour = parseNumber(p, 2);
        if (*p++ != ':')
            return result;
        minute = parseNumber(p, 2);
    }
    if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        return result;
    }
    result.year = year;
    result.month = month;
    result.day = day;
    result.hour = hour;
    result.minute = minute;
    return result;
}

void DsRunnerPlayStats::Load()
{
    if (_loaded)
        return;
    _loaded = true;

    auto file = std::make_unique<File>();
    if (file->Open(PLAY_STATS_PATH, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return;
    // larger files are cut off
    u32 size = std::min<u32>(file->GetSize(), MaxEntries * (MAX_LINE_LENGTH + PLAY_STATS_SUFFIX_SIZE));
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
        // "<path>|<count>|<date>", lines that are no absolute path are skipped
        char* separator = strchr(line, '|');
        if (separator)
            *separator = 0; // before SkipDrive(), the time has a colon too
        const char* path = DsRunnerPaths::SkipDrive(line);
        if (separator && path[0] == '/')
        {
            const char* p = separator + 1;
            int count = parseNumber(p, 4);
            auto& entry = _entries[_count++];
            entry.path = copyString(path, separator - path);
            entry.count = count < 0 ? 0 : count;
            entry.lastPlayed = *p == '|' ? parseDateTime(p + 1) : DsRunnerDateTime();
        }
        line = next != 0 ? end + 1 : end;
    }
    LOG_DEBUG("Play statistics: %d entries\n", _count);
}

void DsRunnerPlayStats::Add(const char* path)
{
    Load();
    path = DsRunnerPaths::SkipDrive(path);

    // take the entry out if it is already in the list, otherwise drop the oldest one
    u32 index = 0;
    while (index < _count && strcasecmp(_entries[index].path.get(), path) != 0)
        index++;
    Entry entry;
    if (index < _count)
    {
        entry = std::move(_entries[index]);
    }
    else
    {
        entry.path = copyString(path, strlen(path));
        if (_count < MaxEntries)
            _count++;
        index = _count - 1;
    }
    for (u32 i = index; i > 0; i--)
        _entries[i] = std::move(_entries[i - 1]);
    if (entry.count < MaxCount)
        entry.count++;
    entry.lastPlayed = DsRunnerDateTime::Now();
    _entries[0] = std::move(entry);

    Save();
}

void DsRunnerPlayStats::Save() const
{
    // the whole file in one write instead of many small ones
    u32 size = 0;
    for (u32 i = 0; i < _count; i++)
        size += strlen(_entries[i].path.get()) + PLAY_STATS_SUFFIX_SIZE + 1;
    auto text = std::make_unique<char[]>(size + 1);
    u32 length = 0;
    for (u32 i = 0; i < _count; i++)
    {
        const auto& entry = _entries[i];
        const auto& date = entry.lastPlayed;
        if (date.IsValid())
        {
            length += mini_snprintf(&text[length], size + 1 - length, "%s|%d|%04d-%02d-%02d %02d:%02d\n",
                entry.path.get(), entry.count, date.year, date.month, date.day, date.hour, date.minute);
        }
        else
        {
            length += mini_snprintf(&text[length], size + 1 - length, "%s|%d|\n", entry.path.get(), entry.count);
        }
    }
    writeTextFile(PLAY_STATS_PATH, text.get(), length);
}

const DsRunnerPlayStats::Entry* DsRunnerPlayStats::Find(const char* folderPath, const char* fileName) const
{
    folderPath = DsRunnerPaths::SkipDrive(folderPath);
    u32 length = folderLength(folderPath);
    for (u32 i = 0; i < _count; i++)
    {
        if (isFileInFolder(_entries[i].path.get(), folderPath, length, fileName))
            return &_entries[i];
    }
    return nullptr;
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

void DsRunnerVirtualEntries::Add(SdFolder& libraryRoot, const char* name)
{
    // a folder entry that exists nowhere on the card: an empty file reference
    DIR directory;
    FILINFO fileInfo;
    memset(&directory, 0, sizeof(directory));
    memset(&fileInfo, 0, sizeof(fileInfo));
    libraryRoot.AddFile(new FileInfo(name, &FolderFileType::sInstance,
        FastFileRef(&directory, &fileInfo), AM_DIR));
}

void DsRunnerVirtualEntries::MoveToFront(const FileInfo** files, u32 count, const char* name)
{
    for (u32 i = 1; i < count; i++)
    {
        if (strcmp(files[i]->GetFileName(), name) == 0)
        {
            const FileInfo* entry = files[i];
            for (u32 j = i; j > 0; j--)
                files[j] = files[j - 1];
            files[0] = entry;
            return;
        }
    }
}

void DsRunnerFavoritesFolder::AddEntry(SdFolder& libraryRoot, const DsRunnerFavorites& favorites)
{
    if (favorites.GetCount() != 0)
        DsRunnerVirtualEntries::Add(libraryRoot, EntryName);
}

void DsRunnerFavoritesFolder::MoveEntryToFront(const FileInfo** files, u32 count)
{
    DsRunnerVirtualEntries::MoveToFront(files, count, EntryName);
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
        // the favorites and search entries are no real folders (the favorites count comes from the favorites)
        if (file->GetFileType()->GetClassification() != FileTypeClassification::Folder ||
            strlen(file->GetFileName()) >= sizeof(Entry::name) ||
            DsRunnerFavoritesFolder::IsEntry(file->GetFileName()) ||
            DsRunnerSearch::IsEntry(file->GetFileName()))
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

int DsRunnerGameCounts::GetTotal(const char* libraryPath) const
{
    if (_libraryPath[0] == 0 || !DsRunnerPaths::IsSameFolder(libraryPath, _libraryPath))
        return Unknown;
    int total = 0;
    for (u32 i = 0; i < _entryCount; i++)
    {
        if (_entries[i].count == Unknown)
            return Unknown;
        total += _entries[i].count;
    }
    return total;
}

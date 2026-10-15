#include "common.h"
#include <string.h>
#include <algorithm>
#include "services/settings/AppSettings.h"
#include "DsRunnerLibrary.h"
#include "DsRunnerNames.h"

// tags after the title: "Title (Europe) (Rev 1) [!]", more than this are left as they are
#define MAX_TAGS            8
// a longer "extension" is part of the name ("Dr. Mario Remastered")
#define MAX_EXTENSION       5

// region names of No-Intro ("(USA, Europe)")
static const char* const sRegions[] =
{
    "World", "Europe", "USA", "Japan", "Germany", "France", "Spain", "Italy", "Netherlands", "Sweden",
    "Norway", "Denmark", "Finland", "Scandinavia", "UK", "Ireland", "Australia", "New Zealand", "Korea",
    "China", "Taiwan", "Hong Kong", "Asia", "Brazil", "Canada", "Mexico", "Latin America", "Russia",
    "Poland", "Portugal", "Greece", "Belgium", "Austria", "Switzerland", "Unknown"
};

static bool isUpper(char c) { return c >= 'A' && c <= 'Z'; }
static bool isLower(char c) { return c >= 'a' && c <= 'z'; }
static bool isDigit(char c) { return c >= '0' && c <= '9'; }

// Length of the name without extension: "Zelda.nes" -> 5. A dot at the start or an "extension"
// with a space or more than MAX_EXTENSION characters doesn't count ("Super Mario Bros. 3").
static u32 getNameLength(const char* fileName)
{
    u32 length = strlen(fileName);
    const char* dot = strrchr(fileName, '.');
    if (!dot || dot == fileName)
        return length;
    u32 extensionLength = length - (dot + 1 - fileName);
    if (extensionLength == 0 || extensionLength > MAX_EXTENSION || memchr(dot + 1, ' ', extensionLength))
        return length;
    return dot - fileName;
}

// One entry of a tag: a region ("Europe"), a language ("En", "Zh-Hant"), a GoodTools region ("U", "JUE")
// or a language count ("M3").
static bool isRemovableTagEntry(const char* entry, u32 length)
{
    if (length == 0)
        return false;
    for (const char* region : sRegions)
    {
        if (strlen(region) == length && strncasecmp(region, entry, length) == 0)
            return true;
    }
    if (length >= 2 && isUpper(entry[0]) && isLower(entry[1]) &&
        (length == 2 || (length >= 4 && entry[2] == '-')))
    {
        for (u32 i = 3; i < length; i++)
        {
            if (!isUpper(entry[i]) && !isLower(entry[i]))
                return false;
        }
        return true;
    }
    if (length <= 3)
    {
        u32 i = 0;
        while (i < length && entry[i] != 0 && strchr("UEJGFSIKABCDHNRW", entry[i]))
            i++;
        if (i == length)
            return true;
    }
    if (entry[0] == 'M' && length <= 3)
    {
        u32 i = 1;
        while (i < length && isDigit(entry[i]))
            i++;
        return i == length && length > 1;
    }
    return false;
}

// \p tag includes the brackets: "[!]", "(USA, Europe)"
static bool isRemovableTag(const char* tag, u32 length)
{
    // dump flags of GoodTools: [!], [b1], [h2C], [T+Ger], ...
    if (tag[0] == '[')
        return true;
    // a list of regions and languages, separated by commas
    const char* p = tag + 1;
    const char* end = tag + length - 1;
    while (p < end)
    {
        while (p < end && *p == ' ')
            p++;
        const char* entryEnd = p;
        while (entryEnd < end && *entryEnd != ',')
            entryEnd++;
        const char* trimmedEnd = entryEnd;
        while (trimmedEnd > p && trimmedEnd[-1] == ' ')
            trimmedEnd--;
        if (!isRemovableTagEntry(p, trimmedEnd - p))
            return false;
        p = entryEnd + 1;
    }
    return true;
}

static void append(char* result, u32 resultSize, u32& position, const char* text, u32 length)
{
    length = std::min(length, resultSize - 1 - position);
    memcpy(result + position, text, length);
    position += length;
}

void DsRunnerNames::GetDisplayName(const char* fileName, bool clean, char* result, u32 resultSize)
{
    if (resultSize == 0)
        return;
    u32 nameLength = getNameLength(fileName);
    u32 position = 0;
    if (!clean)
    {
        append(result, resultSize, position, fileName, nameLength);
        result[position] = 0;
        return;
    }

    // the tags at the end of the name, from the last one to the first
    u32 tagStart[MAX_TAGS];
    u32 tagEnd[MAX_TAGS];
    u32 tagCount = 0;
    u32 titleEnd = nameLength;
    while (titleEnd > 0 && fileName[titleEnd - 1] == ' ')
        titleEnd--;
    while (tagCount < MAX_TAGS && titleEnd > 0 &&
        (fileName[titleEnd - 1] == ')' || fileName[titleEnd - 1] == ']'))
    {
        char open = fileName[titleEnd - 1] == ')' ? '(' : '[';
        u32 start = titleEnd - 1;
        while (start > 0 && fileName[start - 1] != open)
            start--;
        if (start == 0)
            break;
        start--;
        tagStart[tagCount] = start;
        tagEnd[tagCount] = titleEnd;
        tagCount++;
        titleEnd = start;
        while (titleEnd > 0 && fileName[titleEnd - 1] == ' ')
            titleEnd--;
    }
    if (titleEnd == 0)
    {
        // the name is nothing but tags: keep it
        append(result, resultSize, position, fileName, nameLength);
        result[position] = 0;
        return;
    }

    // the title, then the tags that stay
    append(result, resultSize, position, fileName, titleEnd);
    for (int i = tagCount - 1; i >= 0; i--)
    {
        if (isRemovableTag(&fileName[tagStart[i]], tagEnd[i] - tagStart[i]))
            continue;
        append(result, resultSize, position, " ", 1);
        append(result, resultSize, position, &fileName[tagStart[i]], tagEnd[i] - tagStart[i]);
    }
    result[position] = 0;
}

void DsRunnerNames::GetDisplayName(const char* fileName, char* result, u32 resultSize)
{
    const auto* appSettings = gDsRunnerLibrary.appSettings;
    GetDisplayName(fileName, !appSettings || appSettings->cleanNames, result, resultSize);
}

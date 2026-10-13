#include "common.h"
#include <string.h>
#include "romBrowser/Theme/dsrunner/DsRunnerSystems.h"
#include "DsRunnerConsoles.h"

const DsRunnerConsoles::Info* DsRunnerConsoles::FindByFolderName(const char* name)
{
    if (!name)
        return nullptr;
    if (const Info* custom = gDsRunnerSystems.FindCustomByFolderName(name))
        return custom;
    return FindBuiltInByFolderName(name);
}

const DsRunnerConsoles::Info* DsRunnerConsoles::FindByFileName(const char* fileName)
{
    if (!fileName)
        return nullptr;
    const char* extension = strrchr(fileName, '.');
    if (!extension || extension == fileName)
        return nullptr;
    extension++;
    if (const Info* custom = gDsRunnerSystems.FindCustomByExtension(extension))
        return custom;
    return FindBuiltInByFolderName(FindBuiltInFolderByExtension(extension));
}

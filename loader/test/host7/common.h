#pragma once
// Host stand-in for Pico Loader's arm7 common.h with a tiny FatFs on top of stdio, for
// loader/test/igr_log_test.cpp. Paths on the "card" live below IGR_TEST_CARD_DIR.
#include <stdio.h>
#include <string.h>
#include <string>
#include "nds/ndstypes.h"

#define LOG_FATAL(...)   ((void)0)
#define LOG_ERROR(...)   ((void)0)
#define LOG_WARNING(...) ((void)0)
#define LOG_INFO(...)    ((void)0)
#define LOG_DEBUG(...)   ((void)0)
#define LOG_TRACE(...)   ((void)0)

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef enum { FR_OK = 0, FR_DISK_ERR, FR_NO_FILE = 4 } FRESULT;

#define FA_READ             0x01
#define FA_WRITE            0x02
#define FA_OPEN_EXISTING    0x00
#define FA_CREATE_NEW       0x04
#define FA_CREATE_ALWAYS    0x08
#define FA_OPEN_ALWAYS      0x10
#define FA_OPEN_APPEND      0x30

extern std::string gIgrTestCardDir;

struct FIL { FILE* file; };

static inline FRESULT f_open(FIL* fp, const char* path, BYTE mode)
{
    std::string hostPath = gIgrTestCardDir + path;
    const char* hostMode = (mode & FA_CREATE_ALWAYS) ? "wb" : (mode & FA_OPEN_APPEND) == FA_OPEN_APPEND ? "ab" : "rb";
    fp->file = fopen(hostPath.c_str(), hostMode);
    return fp->file ? FR_OK : FR_NO_FILE;
}

static inline FRESULT f_read(FIL* fp, void* buffer, UINT length, UINT* read)
{
    *read = fread(buffer, 1, length, fp->file);
    return FR_OK;
}

static inline FRESULT f_write(FIL* fp, const void* buffer, UINT length, UINT* written)
{
    *written = fwrite(buffer, 1, length, fp->file);
    return FR_OK;
}

static inline FRESULT f_close(FIL* fp)
{
    fclose(fp->file);
    return FR_OK;
}

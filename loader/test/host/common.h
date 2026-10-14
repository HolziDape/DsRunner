#pragma once
// Host stand-in for Pico Loader's arm9 common.h, for loader/test/irq_dispatcher_search_test.cpp.
#include <stdio.h>
#include <string.h>
#include "nds/ndstypes.h"

#define LOG_FATAL(...)   ((void)0)
#define LOG_ERROR(...)   ((void)0)
#define LOG_WARNING(...) ((void)0)
#define LOG_INFO(...)    ((void)0)
#define LOG_DEBUG(...)   ((void)0)
#define LOG_TRACE(...)   ((void)0)

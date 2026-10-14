#pragma once
// Host stand-in for Pico Loader's PatchCode, so InGameResetPatchCode.h compiles.
#include "common.h"

class PatchHeap;

class PatchCode
{
public:
    PatchCode(const void*, u32, PatchHeap&) { }
    const void* GetAddressAtTarget(const void* address) const { return address; }
};

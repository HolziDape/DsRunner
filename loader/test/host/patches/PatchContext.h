#pragma once
// Host stand-in for Pico Loader's PatchContext: only what the irq dispatcher search uses.
#include "common.h"

class IAutoloadAdjuster
{
public:
    virtual ~IAutoloadAdjuster() { }
    virtual u32 AdjustInitialToFinal(u32 initialAddress) const = 0;
};

/// @brief Moves one block like the arm9 autoload moves the ITCM section.
class TestAutoloadAdjuster : public IAutoloadAdjuster
{
public:
    TestAutoloadAdjuster(u32 initialStart, u32 size, u32 finalStart)
        : _initialStart(initialStart), _size(size), _finalStart(finalStart) { }

    u32 AdjustInitialToFinal(u32 initialAddress) const override
    {
        if (initialAddress >= _initialStart && initialAddress < _initialStart + _size)
            return initialAddress - _initialStart + _finalStart;
        return initialAddress;
    }

private:
    u32 _initialStart, _size, _finalStart;
};

class PatchContext
{
public:
    PatchContext(void* data, u32 dataSize, const IAutoloadAdjuster* autoloadAdjuster)
        : _data(data), _dataSize(dataSize), _autoloadAdjuster(autoloadAdjuster) { }

    // same contract as the real one: 16 byte patterns, word aligned
    u32* FindPattern32(const u32* pattern, u32 byteLength) const
    {
        return FindPattern32(pattern, byteLength, GetDataStart());
    }

    u32* FindPattern32(const u32* pattern, u32 byteLength, const u32* searchStart) const
    {
        const u32* end = GetDataEnd();
        if (byteLength != 16 || searchStart < GetDataStart() || searchStart >= end)
            return nullptr;
        for (const u32* p = searchStart; p + 4 <= end; p++)
        {
            if (p[0] == pattern[0] && p[1] == pattern[1] && p[2] == pattern[2] && p[3] == pattern[3])
                return (u32*)p;
        }
        return nullptr;
    }

    const u32* GetDataStart() const { return (const u32*)_data; }
    const u32* GetDataEnd() const { return (const u32*)((const u8*)_data + _dataSize); }
    const IAutoloadAdjuster* GetAutoloadAdjuster() const { return _autoloadAdjuster; }

private:
    void* _data;
    u32 _dataSize;
    const IAutoloadAdjuster* _autoloadAdjuster;
};

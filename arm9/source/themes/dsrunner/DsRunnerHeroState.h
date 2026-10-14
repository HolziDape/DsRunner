#pragma once
#include <string.h>
#include "DsRunnerConsoles.h"

class TaskQueueBase;

/// @brief State shared between the DsRunner theme's file info card (which knows what is selected)
///        and the top screen background (which draws the gradient in the matching color and the artwork).
class DsRunnerHeroState
{
public:
    using Color = DsRunnerConsoles::Color;

    /// @brief Sets the color of the top screen gradient. The background picks it up on the next vblank.
    void SetColor(const Color& color)
    {
        if (color.r != _color.r || color.g != _color.g || color.b != _color.b)
        {
            _color = color;
            _dirty = true;
        }
    }

    const Color& GetColor() const { return _color; }

    /// @brief Returns true once after the color changed.
    bool ConsumeDirty()
    {
        bool dirty = _dirty;
        _dirty = false;
        return dirty;
    }

    /// @brief Sets the artwork to show (nullptr for none) and the queue to load it on.
    /// @param fallbackPath Image shown when \p path doesn't exist (nullptr for none): a template of
    ///        DsRunnerConsoleArt, tinted with \p fallbackTint.
    void SetArt(const char* path, const char* fallbackPath, const Color& fallbackTint, TaskQueueBase* ioTaskQueue)
    {
        if (!path)
            path = "";
        if (!fallbackPath)
            fallbackPath = "";
        if (strcmp(path, _artPath) != 0 || strcmp(fallbackPath, _artFallbackPath) != 0 ||
            fallbackTint.r != _artTint.r || fallbackTint.g != _artTint.g || fallbackTint.b != _artTint.b)
        {
            strlcpy(_artPath, path, sizeof(_artPath));
            strlcpy(_artFallbackPath, fallbackPath, sizeof(_artFallbackPath));
            _artTint = fallbackTint;
            _ioTaskQueue = ioTaskQueue;
            _artDirty = true;
        }
    }

    const char* GetArtPath() const { return _artPath; }
    const char* GetArtFallbackPath() const { return _artFallbackPath; }
    const Color& GetArtTint() const { return _artTint; }
    TaskQueueBase* GetIoTaskQueue() const { return _ioTaskQueue; }

    /// @brief Returns true once after the artwork changed.
    bool ConsumeArtDirty()
    {
        bool dirty = _artDirty;
        _artDirty = false;
        return dirty;
    }

private:
    Color _color = DsRunnerConsoles::kUnknownHero;
    bool _dirty = true;
    char _artPath[256] = { 0 };
    char _artFallbackPath[128] = { 0 };
    Color _artTint;
    TaskQueueBase* _ioTaskQueue = nullptr;
    bool _artDirty = false;
};

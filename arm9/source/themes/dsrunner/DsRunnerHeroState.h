#pragma once
#include "DsRunnerConsoles.h"

/// @brief State shared between the DsRunner theme's file info card (which knows what is selected)
///        and the top screen background (which draws the gradient in the matching color).
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

private:
    Color _color = DsRunnerConsoles::kUnknownHero;
    bool _dirty = true;
};

#pragma once
#include "core/math/Rgb.h"

/// @brief Shapes of the DsRunner theme drawn with the 3D engine (tiles, keys of the search keyboard).
///        Set the polygon attributes and no texture before drawing (see DsRunnerTileItemView::Draw).
namespace DsRunnerShapes
{
    /// @brief Sends one vertex: x and y in 1/64 pixels, z as raw 1.3.12 value (smaller is closer).
    void Vertex(int x64, int y64, int z);

    /// @brief Draws a rounded rectangle as one quad strip from top to bottom with a vertical color gradient.
    /// @param x, y, width, height The rectangle in pixels.
    /// @param radius The corner radius in pixels.
    /// @param clipTop Nothing is drawn above this screen line (the tiles scroll below the app bar).
    void DrawRoundedRect(int x, int y, int width, int height, int radius,
        const Rgb<8, 8, 8>& top, const Rgb<8, 8, 8>& bottom, int z, int clipTop = 0);
}

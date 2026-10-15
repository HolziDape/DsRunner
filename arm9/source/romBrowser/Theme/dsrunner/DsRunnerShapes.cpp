#include "common.h"
#include <algorithm>
#include "gui/Gx.h"
#include "core/math/RgbMixer.h"
#include "DsRunnerShapes.h"

using Rgb888 = Rgb<8, 8, 8>;

// Points of a quarter circle from the top (angle 0) to the side (angle 90 degrees) in steps of 22.5 degrees,
// in 1/256 of the radius: how far down from the top edge (dy) and in from the side edge (dx) they are.
static constexpr int sArcDy[] = { 0, 19, 75, 158, 256 };
static constexpr int sArcDx[] = { 256, 158, 75, 19, 0 };
static constexpr int sArcSteps = sizeof(sArcDy) / sizeof(sArcDy[0]);

void DsRunnerShapes::Vertex(int x64, int y64, int z)
{
    // x is in 1/64 pixels already; the projection has 8 times the resolution on y (see App.cpp)
    REG_GX_VTX_16 = GX_VTX_PACK(x64, y64 >> 3);
    REG_GX_VTX_16 = (u16)z;
}

void DsRunnerShapes::DrawRoundedRect(int x, int y, int width, int height, int radius,
    const Rgb888& top, const Rgb888& bottom, int z, int clipTop)
{
    if (y + height <= clipTop)
        return;

    radius = std::min(radius, std::min(width, height) / 2);
    int left = x << 6;
    int right = (x + width) << 6;
    int r64 = radius << 6;
    int top64 = y << 6;
    int height64 = height << 6;
    int clip64 = (clipTop << 6) - top64;

    Gx::Begin(GX_PRIMITIVE_QUAD_STRIP);
    int previousY = 0;
    int previousDx = 0;
    for (int i = 0; i < sArcSteps * 2; i++)
    {
        // first the arc of the top corners, then the arc of the bottom corners mirrored
        int step = i < sArcSteps ? i : (sArcSteps * 2 - 1 - i);
        int dy = (sArcDy[step] * r64) >> 8;
        int dx = (sArcDx[step] * r64) >> 8;
        int lineY = i < sArcSteps ? dy : (height64 - dy);

        if (lineY < clip64)
        {
            previousY = lineY;
            previousDx = dx;
            continue;
        }
        if (i > 0 && previousY < clip64)
        {
            // the first line below the clip line: start at the clip line, on the edge of the shape
            int clipDx = lineY == previousY ? dx
                : previousDx + (dx - previousDx) * (clip64 - previousY) / (lineY - previousY);
            Gx::Color(Rgb<5, 5, 5>(RgbMixer::Lerp(top, bottom, clip64, height64)));
            Vertex(right - clipDx, top64 + clip64, z);
            Vertex(left + clipDx, top64 + clip64, z);
            previousY = clip64;
        }

        Gx::Color(Rgb<5, 5, 5>(RgbMixer::Lerp(top, bottom, lineY, height64)));
        Vertex(right - dx, top64 + lineY, z);
        Vertex(left + dx, top64 + lineY, z);
    }
    Gx::End();
}

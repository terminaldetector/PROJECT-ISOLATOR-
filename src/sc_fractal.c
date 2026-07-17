#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"

// scene 4: the city collapses into pure recursion - sierpinski bloom

static u8 maxDepth;

static void tri(s16 x1, s16 y1, s16 x2, s16 y2, s16 x3, s16 y3, u8 d)
{
    if (d == 0)
    {
        u8 col = 2 + (maxDepth & 3);
        bmp_lineSafe(x1, y1, x2, y2, col);
        bmp_lineSafe(x2, y2, x3, y3, col);
        bmp_lineSafe(x3, y3, x1, y1, col);
        return;
    }
    s16 mx1 = (x1 + x2) >> 1, my1 = (y1 + y2) >> 1;
    s16 mx2 = (x2 + x3) >> 1, my2 = (y2 + y3) >> 1;
    s16 mx3 = (x3 + x1) >> 1, my3 = (y3 + y1) >> 1;
    tri(x1, y1, mx1, my1, mx3, my3, d - 1);
    tri(mx1, my1, x2, y2, mx2, my2, d - 1);
    tri(mx3, my3, mx2, my2, x3, y3, d - 1);
}

void fractal_init(void)
{
    demo_enterBMP();

    PAL_setColor(16 + 0, 0x0000);
    PAL_setColor(16 + 1, VCOL(1, 0, 3));
    PAL_setColor(16 + 2, VCOL(3, 1, 6));
    PAL_setColor(16 + 3, VCOL(5, 3, 7));
    PAL_setColor(16 + 4, VCOL(7, 5, 3));
    PAL_setColor(16 + 5, VCOL(2, 6, 5));
    PAL_setColor(16 + 15, VCOL(7, 7, 7));
    VDP_setBackgroundColor(16);

    snd_setMood(SND_CALM);
}

void fractal_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    // recursion deepens over time: 1 -> 3 -> 9 -> 27 triangles
    maxDepth = t / 170;
    if (maxDepth > 3) maxDepth = 3;

    // the whole fractal breathes and rotates
    u16 rot = t;
    s16 r = 74 + (SIN(t * 3) >> 4);

    s16 x[3], y[3];
    for (u16 i = 0; i < 3; i++)
    {
        u16 a = rot + 192 + i * 85;
        x[i] = 128 + ((r * COS(a)) >> 8);
        y[i] = 80 - ((r * SIN(a)) >> 8);
    }
    tri(x[0], y[0], x[1], y[1], x[2], y[2], maxDepth);

    // ghost copy, counter-rotating
    if (t > 300)
    {
        s16 r2 = r >> 1;
        for (u16 i = 0; i < 3; i++)
        {
            u16 a = (u16)(-rot) + 64 + i * 85;
            x[i] = 128 + ((r2 * COS(a)) >> 8);
            y[i] = 80 - ((r2 * SIN(a)) >> 8);
        }
        tri(x[0], y[0], x[1], y[1], x[2], y[2], maxDepth > 1 ? 1 : maxDepth);
    }

    // depth-jump flash
    if ((t % 170) < 4 && t > 100) fx_fillPal(16, 1, VCOL(2, 2, 3));
    else if ((t % 170) == 4) PAL_setColor(16, 0x0000);

    BMP_flip(1);

    // hue rotation across the recursion levels
    if ((t & 3) == 0)
    {
        PAL_setColor(16 + 2, fx_hue(t & 255, 5));
        PAL_setColor(16 + 3, fx_hue((t + 60) & 255, 7));
        PAL_setColor(16 + 5, fx_hue((t + 140) & 255, 6));
    }
}

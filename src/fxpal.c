#include "fxpal.h"
#include "util.h"

static u16 buf[64];

static u16 lerpCol(u16 c1, u16 c2, u16 t, u16 tmax)
{
    u16 r1 = (c1 >> 1) & 7, g1 = (c1 >> 5) & 7, b1 = (c1 >> 9) & 7;
    u16 r2 = (c2 >> 1) & 7, g2 = (c2 >> 5) & 7, b2 = (c2 >> 9) & 7;
    u16 r = r1 + ((r2 - r1) * t) / tmax;
    u16 g = g1 + ((g2 - g1) * t) / tmax;
    u16 b = b1 + ((b2 - b1) * t) / tmax;
    return VCOL(r & 7, g & 7, b & 7);
}

void fx_gradient(u16 index, u16 count, u16 c1, u16 c2)
{
    for (u16 i = 0; i < count; i++)
        buf[i] = lerpCol(c1, c2, i, count - 1);
    PAL_setColors(index, buf, count, CPU);
}

void fx_fillPal(u16 index, u16 count, u16 color)
{
    for (u16 i = 0; i < count; i++) buf[i] = color;
    PAL_setColors(index, buf, count, CPU);
}

void fx_allBlack(void)
{
    fx_fillPal(0, 64, 0);
}

void fx_allWhite(void)
{
    fx_fillPal(0, 64, VCOL(7, 7, 7));
}

u16 fx_hue(u8 h, u16 bright)
{
    // triangle waves over the hue circle, scaled to 0..bright
    s16 r = SIN(h);
    s16 g = SIN(h + 85);
    s16 b = SIN(h + 171);
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    r = (r * bright) >> 8;
    g = (g * bright) >> 8;
    b = (b * bright) >> 8;
    return VCOL(r, g, b);
}

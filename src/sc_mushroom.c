#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"

// scene 9: procedural nuclear bloom - flash, stem, cap, shockwave

#define GY 150      // ground line

void mushroom_init(void)
{
    demo_enterBMP();

    // start as a blinding flash
    fx_allWhite();
    snd_boom();
}

static void heatPalette(u16 age)
{
    // white -> yellow -> orange -> red -> darkness, cooling with age
    for (u16 i = 1; i < 16; i++)
    {
        s16 heat = (15 - i) + 8 - (age >> 6);
        if (heat < 0) heat = 0;
        u16 r = heat > 4 ? 7 : heat + 2;
        u16 g = heat > 8 ? 7 : heat >> 1;
        u16 b = heat > 12 ? heat - 10 : 0;
        if (r > 7) r = 7;
        if (g > 7) g = 7;
        PAL_setColor(16 + i, VCOL(r, g, b));
    }
    PAL_setColor(16, 0x0000);
    VDP_setBackgroundColor(16);
}

void mushroom_update(u16 t)
{
    if (t < 14)
        return;             // hold the white flash
    if (t == 14)
        heatPalette(0);

    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    u16 age = t - 14;

    // growth curves
    s16 hs = age;                       // stem height
    if (hs > 92) hs = 92;
    s16 sw = 8 + (age >> 3);            // stem half width
    if (sw > 20) sw = 20;
    s16 cr = 12 + (age >> 1);           // cap radius
    if (cr > 58) cr = 58;

    s16 capY = GY - hs;

    // ground
    bmp_lineSafe(0, GY, 255, GY, 3);

    // stem: dense vertical strokes, brighter in the core
    for (s16 x = -sw; x <= sw; x += 2)
    {
        u8 col = (x < -(sw >> 1) || x > (sw >> 1)) ? 5 : 12;
        s16 wob = (SIN((age << 2) + (x << 3)) >> 6);
        bmp_lineSafe(128 + x + wob, GY, 128 + x, capY + (cr >> 2), col);
    }

    // cap: radial fan, filled look, rolling edges
    for (u16 i = 0; i <= 32; i++)
    {
        u16 a = (i << 2);               // 0..128 - upper half circle
        s16 rr = cr + (SIN((age << 1) + (i << 4)) >> 5);
        u8 col = 4 + ((i * 11) >> 5);
        bmp_lineSafe(128, capY, 128 + ((rr * COS(a)) >> 8),
                     capY - ((rr * SIN(a)) >> 8) / 2, col);
    }
    // cap rim curl
    for (u16 i = 0; i < 16; i++)
    {
        u16 a1 = i << 3, a2 = (i + 1) << 3;
        bmp_lineSafe(128 + ((cr * COS(a1)) >> 8), capY - (((cr * SIN(a1)) >> 8) >> 1),
                     128 + ((cr * COS(a2)) >> 8), capY - (((cr * SIN(a2)) >> 8) >> 1), 14);
    }

    // shockwave ellipse expanding along the ground
    s16 srx = age << 1;
    s16 sry = age / 3;
    if (srx < 250)
        for (u16 i = 0; i < 16; i++)
        {
            u16 a1 = i << 4, a2 = (i + 1) << 4;
            bmp_lineSafe(128 + ((srx * COS(a1)) >> 8), GY - ((sry * SIN(a1)) >> 8),
                         128 + ((srx * COS(a2)) >> 8), GY - ((sry * SIN(a2)) >> 8), 15);
        }

    // debris and sparks around the cap
    for (u16 i = 0; i < 6; i++)
    {
        s16 dx = (s16)(rnd() & 127) - 64;
        s16 dy = (s16)(rnd() & 63) - 40;
        BMP_setPixel(128 + dx, capY + dy, BCOL(10 + (rnd() & 5)));
    }

    BMP_flip(1);

    // the fireball cools over time
    if ((age & 31) == 0) heatPalette(age);

    // ground rumble aftershocks
    if (age == 180 || age == 360) snd_boom();
}

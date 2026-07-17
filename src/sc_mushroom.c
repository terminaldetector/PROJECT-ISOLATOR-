#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"
#include "seq.h"

// scene 9: procedural nuclear bloom, beat-locked.
// the detonation waits for the downbeat; the shockwave rides the bars,
// aftershocks land on the snare. dithered fireball flesh.

#define GY 150

static u16 detonated;
static u16 detFrame;

void mushroom_init(void)
{
    demo_enterBMP();
    fx_allWhite();
    detonated = FALSE;
    detFrame = 0;
    snd_setMood(SND_HARD);
}

static void heatPalette(u16 age)
{
    for (u16 i = 1; i < 16; i++)
    {
        s16 heat = (15 - i) + 8 - (age >> 6);
        if (heat < 0) heat = 0;
        u16 r = heat > 4 ? 7 : heat + 2;
        u16 g = heat > 8 ? 7 : heat >> 1;
        u16 b = heat > 12 ? heat - 10 : 0;
        if (r > 7) r = 7;
        if (g > 7) g = 7;
        if (b > 7) b = 7;
        PAL_setColor(16 + i, VCOL(r, g, b));
    }
    PAL_setColor(16, 0x0000);
    VDP_setBackgroundColor(16);
}

void mushroom_update(u16 t)
{
    // hold the blinding flash until the music says NOW
    if (!detonated)
    {
        if ((t > 8 && seq_isDownbeat()) || t > 120)
        {
            detonated = TRUE;
            heatPalette(0);
            snd_boom();
        }
        return;
    }

    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    u16 age = detFrame++;

    s16 hs = age;
    if (hs > 92) hs = 92;
    s16 sw = 8 + (age >> 3);
    if (sw > 20) sw = 20;
    s16 cr = 12 + (age >> 1);
    if (cr > 58) cr = 58;

    s16 capY = GY - hs;

    bmp_lineSafe(0, GY, 255, GY, 3);

    // stem: dithered column of fire
    for (s16 x = -sw; x <= sw; x += 2)
    {
        u8 col = (x < -(sw >> 1) || x > (sw >> 1)) ? 5 : 12;
        s16 wob = (SIN((age << 2) + (x << 3)) >> 6);
        bmp_lineSafe(128 + x + wob, GY, 128 + x, capY + (cr >> 2), col);
    }

    // cap: solid dithered dome, brighter core - real fireball flesh
    for (s16 yy = -(cr >> 1); yy <= 0; yy += 1)
    {
        s16 d = -yy;
        s16 v = (cr >> 1) * (cr >> 1) - d * d;
        s16 w = cr;
        while ((w * w) >> 2 > v && w > 0) w--;
        u8 inner = 12 - (d >> 3);
        u8 outer = 6 + ((age >> 5) & 1);
        bmp_hspan(128 - w, 128 + w, capY + yy,
                  (yy & 1) ? BCOL2(inner, outer) : BCOL2(outer, inner));
    }
    // rolling rim
    for (u16 i = 0; i < 16; i++)
    {
        u16 a1 = i << 3, a2 = (i + 1) << 3;
        s16 rr = cr + (SIN((age << 1) + (i << 4)) >> 5);
        bmp_lineSafe(128 + ((rr * COS(a1)) >> 8), capY - (((rr * SIN(a1)) >> 8) >> 1),
                     128 + ((rr * COS(a2)) >> 8), capY - (((rr * SIN(a2)) >> 8) >> 1), 14);
    }

    // shockwave: a ring per bar, expanding with the music
    u16 ringBase = (seq_bar() & 3) * 24;
    s16 srx = (age << 1) + ringBase;
    s16 sry = (age / 3) + (ringBase >> 3);
    if (srx < 260)
        for (u16 i = 0; i < 16; i++)
        {
            u16 a1 = i << 4, a2 = (i + 1) << 4;
            bmp_lineSafe(128 + ((srx * COS(a1)) >> 8), GY - ((sry * SIN(a1)) >> 8),
                         128 + ((srx * COS(a2)) >> 8), GY - ((sry * SIN(a2)) >> 8), 15);
        }

    // debris storm
    for (u16 i = 0; i < 6; i++)
    {
        s16 ddx = (s16)(rnd() & 127) - 64;
        s16 ddy = (s16)(rnd() & 63) - 40;
        BMP_setPixel(128 + ddx, capY + ddy, BCOL(10 + (rnd() & 5)));
    }

    BMP_flip(1);

    if ((age & 31) == 0) heatPalette(age);

    // aftershocks ride the snare
    if (seq_isSnare() && age > 120) snd_boom();
    // white flicker on the kick - the sky still burns
    if (seq_isKick()) PAL_setColor(16, VCOL(2, 2, 2));
    else if ((t & 7) == 0) PAL_setColor(16, 0x0000);
}

#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"

// scene 2: emptiness -> grid -> one triangle -> the eye -> rotation begins

#define HORIZON  64

static void drawGrid(u16 t, u8 col)
{
    // receding floor grid with a vanishing point
    u16 rows = (t > 150) ? 9 : (t / 17);
    for (u16 i = 1; i <= rows; i++)
    {
        s16 y = HORIZON + (96 * 16) / (16 + (9 - i) * 12);
        bmp_lineSafe(0, y, 255, y, col);
    }
    u16 cols = (t > 150) ? 11 : (t / 14);
    for (u16 i = 0; i < cols; i++)
    {
        s16 x = -160 + i * 32;
        bmp_lineSafe(128 + x, 159, 128 + (x >> 3), HORIZON, col);
    }
}

static void emblem(s16 cx, s16 cy, s16 r, u16 rot, u16 eyeOpen, u8 col, u8 eyeCol)
{
    // triangle
    s16 tx[3], ty[3];
    for (u16 i = 0; i < 3; i++)
    {
        u16 a = rot + 192 + i * 85;
        tx[i] = cx + ((r * COS(a)) >> 8);
        ty[i] = cy - ((r * SIN(a)) >> 8);
    }
    bmp_lineSafe(tx[0], ty[0], tx[1], ty[1], col);
    bmp_lineSafe(tx[1], ty[1], tx[2], ty[2], col);
    bmp_lineSafe(tx[2], ty[2], tx[0], ty[0], col);

    if (!eyeOpen) return;

    // the eye: almond outline + iris
    s16 rx = (r * 5) >> 3;
    s16 ry = ((r * 5) >> 3) * eyeOpen / 32;
    s16 ey = cy + (r >> 3);
    s16 px = cx, py = ey, fx, fy;
    for (u16 i = 0; i <= 12; i++)
    {
        u16 a = (i * 256) / 12;
        s16 x = cx + ((rx * COS(a)) >> 8);
        s16 y = ey - ((ry * SIN(a)) >> 8);
        if (i == 0) { fx = x; fy = y; }
        else bmp_lineSafe(px, py, x, y, eyeCol);
        px = x; py = y;
    }
    (void) fx; (void) fy;

    // iris: dithered solid disc with a dark pupil core
    s16 ir = ry >> 1;
    if (ir > 2)
    {
        bmp_disc(cx, ey, ir, 15, 4);
        bmp_disc(cx, ey, ir >> 1, 0, 0);
    }
}

void birth_init(void)
{
    demo_enterBMP();

    // cold cyan world being born
    PAL_setColor(16 + 0, 0x0000);
    PAL_setColor(16 + 1, VCOL(0, 1, 2));      // dim grid
    PAL_setColor(16 + 2, VCOL(0, 3, 4));      // grid
    PAL_setColor(16 + 3, VCOL(1, 5, 6));      // bright lines
    PAL_setColor(16 + 4, VCOL(5, 2, 6));      // magenta accent
    PAL_setColor(16 + 15, VCOL(7, 7, 7));     // white
    VDP_setBackgroundColor(16);

    snd_setMood(SND_CALM);
}

void birth_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    if (t > 40) drawGrid(t - 40, (t < 120) ? 1 : 2);

    if (t >= 180 && t < 560)
    {
        // triangle grows, then the eye opens
        s16 r = (t - 180) / 3;
        if (r > 46) r = 46;
        u16 eye = 0;
        if (t > 330)
        {
            eye = (t - 330) / 4;
            if (eye > 32) eye = 32;
            // blink once at ~t 500
            if (t > 490 && t < 520) eye = (t < 505) ? (520 - t) : (t - 505) * 2;
            if (eye > 32) eye = 32;
        }
        emblem(128, 74, r, 0, eye, 3, 4);
    }
    else if (t >= 560)
    {
        // the emblem starts to rotate in 3D - the camera wakes up
        u16 spin = (t - 560) * 2;
        d3_setCamera(8, spin, 190);
        V3 a, b;
        s16 r = 46;
        for (u16 i = 0; i < 3; i++)
        {
            u16 a1 = 192 + i * 85;
            u16 a2 = 192 + ((i + 1) % 3) * 85;
            a.x = (r * COS(a1)) >> 8;  a.y = (r * SIN(a1)) >> 8;  a.z = 0;
            b.x = (r * COS(a2)) >> 8;  b.y = (r * SIN(a2)) >> 8;  b.z = 0;
            d3_line(&a, &b, 3);
        }
        // eye ring in 3D
        for (u16 i = 0; i < 8; i++)
        {
            u16 a1 = (i * 256) / 8, a2 = ((i + 1) * 256) / 8;
            a.x = (18 * COS(a1)) >> 8;  a.y = -6 + ((12 * SIN(a1)) >> 8);  a.z = 0;
            b.x = (18 * COS(a2)) >> 8;  b.y = -6 + ((12 * SIN(a2)) >> 8);  b.z = 0;
            d3_line(&a, &b, 4);
        }
    }

    // sparse "data" pixels shimmer into existence
    if (t > 100)
        for (u16 i = 0; i < 4; i++)
            BMP_setPixel(rnd() & 255, rnd_range(160), BCOL(1));

    BMP_flip(1);

    // world brightens as it forms
    if ((t & 31) == 0 && t < 256)
        PAL_setColor(16 + 1, VCOL(0, 1 + (t >> 7), 2));
}

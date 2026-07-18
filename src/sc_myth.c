#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "fxhint.h"
#include "draw3d.h"
#include "sound.h"
#include "seq.h"

// scene 15, the closing of the circle: a single spark falls from the
// night sky - swaying like the flower did - touches the ground, and
// ignites the next generated world. MYTH.

#define TRAIL 24

static s16 trailX[TRAIL], trailY[TRAIL];
static u16 trailHead;
static u16 landed;
static u16 landFrame;

void myth_init(void)
{
    demo_enterBMP();

    fx_allBlack();
    VDP_setBackgroundColor(16);
    PAL_setColor(16, VCOL(0, 0, 1));        // night sky (BMP clear color)

    PAL_setColor(16 + 10, VCOL(7, 6, 2));
    PAL_setColor(16 + 11, VCOL(7, 4, 0));
    PAL_setColor(16 + 12, VCOL(5, 2, 0));
    PAL_setColor(16 + 13, VCOL(3, 1, 0));
    PAL_setColor(16 + 14, VCOL(3, 3, 5));
    PAL_setColor(16 + 15, VCOL(7, 7, 7));

    for (u16 i = 0; i < TRAIL; i++) { trailX[i] = -10; trailY[i] = -10; }
    trailHead = 0;
    landed = FALSE;
    landFrame = 0;

    snd_setMood(SND_MYTHIC);
}

void myth_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    // stars
    for (u16 i = 0; i < 3; i++)
        BMP_setPixel(rnd() & 255, rnd_range(130), BCOL(14));

    // the ground: a thin dark line of the world-to-be
    bmp_lineSafe(0, 146, 255, 146, 12);

    if (!landed)
    {
        // the spark falls the way the flower fell - a pendulum drift
        s16 sy = -8 + (t / 5);
        s16 sx = 128 + (SIN(t >> 1) >> 2) + (SIN((t << 1) + 40) >> 5);
        if (sy >= 144)
        {
            landed = TRUE;
            landFrame = t;
            snd_boom();
        }

        trailX[trailHead % TRAIL] = sx;
        trailY[trailHead % TRAIL] = sy;
        trailHead++;

        // ember trail, cooling along its length
        for (u16 o = 0; o < TRAIL; o++)
        {
            u16 idx = (trailHead + TRAIL - 1 - o) % TRAIL;
            s16 px = trailX[idx];
            s16 py = trailY[idx];
            if (py < -8) continue;
            u8 col = (o < 4) ? 10 : (o < 10) ? 11 : (o < 16) ? 12 : 13;
            BMP_setPixel(px, py, BCOL(col));
            if (o < 4)
            {
                BMP_setPixel(px + 1, py, BCOL(col));
                BMP_setPixel(px, py + 1, BCOL(col));
            }
        }

        // the spark itself flares with the kick
        u8 core = seq_isKick() ? 15 : 10;
        bmp_lineSafe(sx - 2, sy, sx + 2, sy, core);
        bmp_lineSafe(sx, sy - 2, sx, sy + 2, core);
        BMP_setPixel(sx, sy, BCOL(15));
    }
    else
    {
        // ignition: rings of fire spread from the landing point
        u16 age = t - landFrame;
        for (u16 k = 0; k < 3; k++)
        {
            s16 r = (age << 1) - k * 14;
            if (r < 2 || r > 200) continue;
            for (u16 i = 0; i < 16; i++)
            {
                u16 a1 = i << 4, a2 = (i + 1) << 4;
                bmp_lineSafe(128 + ((r * COS(a1)) >> 8), 146 - (((r * SIN(a1)) >> 8) >> 2),
                             128 + ((r * COS(a2)) >> 8), 146 - (((r * SIN(a2)) >> 8) >> 2),
                             10 + k);
            }
        }
        // rising embers
        for (u16 i = 0; i < age >> 2 && i < 12; i++)
            BMP_setPixel(128 + (s16)(rnd() & 63) - 32, 144 - rnd_range(age), BCOL(11));

        // the world catches: whiteout via the BMP background (CRAM 16),
        // which BMP_clear floods the screen with each frame
        if (age > 60)
        {
            u16 v = (age - 60) >> 3;
            if (v > 7) v = 7;
            PAL_setColor(16, VCOL(v, v, v));
        }
    }

    // MYTH - letter by letter, thick strokes at the bottom of the frame
    u16 letters = t / 90;
    if (letters > 4) letters = 4;
    s16 lx = 96;
    if (letters > 0) // M
    {
        bmp_lineSafe(lx, 156, lx, 148, 15); bmp_lineSafe(lx, 148, lx + 4, 152, 15);
        bmp_lineSafe(lx + 4, 152, lx + 8, 148, 15); bmp_lineSafe(lx + 8, 148, lx + 8, 156, 15);
    }
    lx += 16;
    if (letters > 1) // Y
    {
        bmp_lineSafe(lx, 148, lx + 4, 152, 15); bmp_lineSafe(lx + 8, 148, lx + 4, 152, 15);
        bmp_lineSafe(lx + 4, 152, lx + 4, 156, 15);
    }
    lx += 16;
    if (letters > 2) // T
    {
        bmp_lineSafe(lx, 148, lx + 8, 148, 15); bmp_lineSafe(lx + 4, 148, lx + 4, 156, 15);
    }
    lx += 16;
    if (letters > 3) // H
    {
        bmp_lineSafe(lx, 148, lx, 156, 15); bmp_lineSafe(lx + 8, 148, lx + 8, 156, 15);
        bmp_lineSafe(lx, 152, lx + 8, 152, 15);
    }

    BMP_flip(1);
}

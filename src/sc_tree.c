#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "fxhint.h"
#include "draw3d.h"
#include "sound.h"
#include "seq.h"

// scene 13: out of the white, the tree of life.
// a living fractal tree grows both ways - roots and crown - while the
// ten sephirot and twenty-two paths surface over it. sun and moon face
// each other across the sky like the two pillars, jachin and boaz.

// canonical sephirot layout (x: 0..255, y: 0..159), top = keter
static const s16 sefX[10] = { 128,  88, 168,  88, 168, 128,  88, 168, 128, 128 };
static const s16 sefY[10] = {  18,  38,  38,  68,  68,  58,  98,  98, 108, 138 };

static const u8 paths[22][2] =
{
    {0,1},{0,2},{1,2},{1,3},{2,4},{1,5},{2,5},{3,4},{3,5},{4,5},
    {3,6},{4,7},{5,6},{5,7},{6,7},{6,8},{7,8},{5,8},{6,9},{7,9},
    {8,9},{0,5},
};

static void crescent(s16 cx, s16 cy, s16 r, u8 col)
{
    for (s16 yy = -r; yy <= r; yy++)
    {
        s16 v = r * r - yy * yy;
        s16 w = (v > 0) ? (s16) isqrt32((u32) v) : 0;
        s16 v2 = (r - 3) * (r - 3) - (yy - 2) * (yy - 2);
        s16 w2 = (v2 > 0) ? (s16) isqrt32((u32) v2) : 0;
        // outer disc minus an offset inner disc = the crescent
        s16 xr = cx + w, xl = cx + w2 - 4;
        if (xl < cx - w) xl = cx - w;
        if (xr > xl) bmp_hspan(xl, xr, cy + yy, BCOL(col));
    }
}

static void pillar(s16 x, u8 col)
{
    bmp_lineSafe(x - 3, 150, x - 3, 60, col);
    bmp_lineSafe(x + 3, 150, x + 3, 60, col);
    bmp_lineSafe(x - 6, 60, x + 6, 60, col);
    bmp_lineSafe(x - 6, 57, x + 6, 57, col);
    bmp_lineSafe(x - 8, 150, x + 8, 150, col);
}

void tree_init(void)
{
    demo_enterBMP();

    fx_allWhite();
    VDP_setBackgroundColor(16);

    PAL_setColor(16 + 2, VCOL(3, 2, 0));    // bark
    PAL_setColor(16 + 3, VCOL(4, 3, 1));
    PAL_setColor(16 + 4, VCOL(5, 4, 1));
    PAL_setColor(16 + 5, VCOL(6, 5, 2));
    PAL_setColor(16 + 6, VCOL(7, 6, 3));
    PAL_setColor(16 + 7, VCOL(7, 7, 4));
    PAL_setColor(16 + 8, VCOL(2, 1, 3));    // roots violet
    PAL_setColor(16 + 9, VCOL(1, 3, 1));
    PAL_setColor(16 + 10, VCOL(7, 6, 0));   // sephirot gold
    PAL_setColor(16 + 11, VCOL(7, 4, 0));
    PAL_setColor(16 + 12, VCOL(6, 6, 7));   // moon silver
    PAL_setColor(16 + 13, VCOL(7, 3, 1));   // sun blood
    PAL_setColor(16 + 14, VCOL(3, 3, 5));
    PAL_setColor(16 + 15, VCOL(7, 7, 7));

    snd_setMood(SND_MYTHIC);
}

void tree_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    // the white burns away into deep night over the first seconds
    // (the BMP background is CRAM 16, which BMP_clear fills each frame)
    if (t < 140)
    {
        u16 v = 7 - (t / 20);
        PAL_setColor(16, VCOL(v > 2 ? v - 2 : 0, v > 2 ? v - 2 : 0, v));
    }
    else
    {
        // starry night: near-black indigo, breathing gently with the bar
        u16 breathe = (seq_step() < 8) ? 1 : 0;
        PAL_setColor(16, VCOL(0, 0, 1 + breathe));
    }

    // stars
    if (t > 100)
        for (u16 i = 0; i < 4; i++)
            BMP_setPixel(rnd() & 255, rnd_range(150), BCOL((rnd() & 1) ? 14 : 15));

    // ---- the living tree: crown up, roots down ----
    u16 lt = (t > 60) ? t - 60 : 0;
    u8 grow = 1 + (lt / 45);
    if (grow > 7) grow = 7;
    s16 len = 20 + (lt >> 4);
    if (len > 34) len = 34;
    // trunk
    bmp_lineSafe(128, 150, 128, 150 - len, 2);
    bmp_lineSafe(127, 150, 127, 150 - len, 2);
    frac_branch(128, 150 - len, 64, len, 7, grow, t >> 1);
    // roots mirror the crown, half depth
    frac_branch(128, 150, 192, (len * 2) / 3, 5, grow > 2 ? grow - 2 : 1, t >> 2);

    // ---- jachin and boaz: sun and moon facing each other ----
    if (t > 200)
    {
        pillar(34, 6);
        pillar(222, 12);
        u16 rise = (t - 200) >> 3;
        if (rise > 16) rise = 16;
        bmp_disc(34, 46 - (rise >> 1), 10 + (rise >> 2), 13, 11);   // the sun
        // sun rays
        for (u16 i = 0; i < 8; i++)
        {
            u16 a = (i << 5) + (t >> 2);
            bmp_lineSafe(34 + ((14 * COS(a)) >> 8), 46 - (rise >> 1) - ((14 * SIN(a)) >> 8),
                         34 + ((19 * COS(a)) >> 8), 46 - (rise >> 1) - ((19 * SIN(a)) >> 8), 13);
        }
        crescent(222, 46 - (rise >> 1), 10, 12);                    // the moon
    }

    // ---- the sephirot surface, one per half-bar ----
    if (t > 300)
    {
        u16 lit = (t - 300) / 40;
        if (lit > 10) lit = 10;
        // paths first, faint gold
        for (u16 p = 0; p < 22; p++)
        {
            if (paths[p][0] < lit && paths[p][1] < lit)
                bmp_lineSafe(sefX[paths[p][0]], sefY[paths[p][0]],
                             sefX[paths[p][1]], sefY[paths[p][1]], 11);
        }
        for (u16 s = 0; s < lit; s++)
        {
            u8 hot = (seq_isKick() && (seq_bar() % 10) == s) ? 15 : 10;
            bmp_disc(sefX[s], sefY[s], 6, hot, 11);
        }
    }

    BMP_flip(1);

    // gold shimmer on the downbeat
    if (seq_isDownbeat())
        PAL_setColor(16 + 10, VCOL(7, 7, 2));
    else if ((t & 15) == 0)
        PAL_setColor(16 + 10, VCOL(7, 6, 0));
}

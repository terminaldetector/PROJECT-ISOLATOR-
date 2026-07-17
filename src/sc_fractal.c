#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"

// scene 4: pure recursion, three movements -
// sierpinski bloom -> a living fractal tree -> the menger cube

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

// shared with the tree-of-life scene: recursive branching
void frac_branch(s16 x, s16 y, u16 ang, s16 len, u8 depth, u8 grow, u16 sway)
{
    if (depth == 0 || len < 3) return;
    s16 ex = x + ((len * COS(ang)) >> 8);
    s16 ey = y - ((len * SIN(ang)) >> 8);
    bmp_lineSafe(x, y, ex, ey, 2 + (depth & 7));
    if (grow == 0) return;
    u16 s = SIN(sway + (depth << 4)) >> 5;
    frac_branch(ex, ey, ang + 24 + s, (len * 2) / 3, depth - 1, grow - 1, sway);
    frac_branch(ex, ey, ang - 26 + s, (len * 2) / 3, depth - 1, grow - 1, sway);
    if (depth > 4)
        frac_branch(ex, ey, ang + s, (len * 3) / 5, depth - 2, grow - 1, sway);
}

// sierpinski carpet on a projected cube face (bilinear-approximated)
static void carpet(const s16 *fx, const s16 *fy, u8 depth)
{
    s16 gx[4][4], gy[4][4];
    for (u16 j = 0; j < 4; j++)
        for (u16 i = 0; i < 4; i++)
        {
            s16 topx = fx[0] + (((fx[1] - fx[0]) * (s16) i) / 3);
            s16 topy = fy[0] + (((fy[1] - fy[0]) * (s16) i) / 3);
            s16 botx = fx[3] + (((fx[2] - fx[3]) * (s16) i) / 3);
            s16 boty = fy[3] + (((fy[2] - fy[3]) * (s16) i) / 3);
            gx[j][i] = topx + (((botx - topx) * (s16) j) / 3);
            gy[j][i] = topy + (((boty - topy) * (s16) j) / 3);
        }
    // the hole
    bmp_fillTri(gx[1][1], gy[1][1], gx[1][2], gy[1][2], gx[2][2], gy[2][2], 0, 0);
    bmp_fillTri(gx[1][1], gy[1][1], gx[2][2], gy[2][2], gx[2][1], gy[2][1], 0, 0);

    if (depth > 1)
    {
        // recurse into the 8 surrounding cells
        for (u16 j = 0; j < 3; j++)
            for (u16 i = 0; i < 3; i++)
            {
                if (i == 1 && j == 1) continue;
                s16 cfx[4] = { gx[j][i], gx[j][i + 1], gx[j + 1][i + 1], gx[j + 1][i] };
                s16 cfy[4] = { gy[j][i], gy[j][i + 1], gy[j + 1][i + 1], gy[j + 1][i] };
                carpet(cfx, cfy, depth - 1);
            }
    }
}

static void mengerCube(u16 t)
{
    static const V3 corners[8] =
    {
        { -50, -50, -50 }, { 50, -50, -50 }, { 50, 50, -50 }, { -50, 50, -50 },
        { -50, -50,  50 }, { 50, -50,  50 }, { 50, 50,  50 }, { -50, 50,  50 },
    };
    static const u8 faces[6][4] =
    {
        { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 },
        { 1, 5, 6, 2 }, { 4, 5, 1, 0 }, { 3, 2, 6, 7 },
    };

    d3_setCamera(t, (t << 1), 230);

    s16 sx[8], sy[8];
    u16 ok = TRUE;
    for (u16 i = 0; i < 8; i++)
        if (!d3_project(&corners[i], &sx[i], &sy[i])) ok = FALSE;
    if (!ok) return;

    for (u16 f = 0; f < 6; f++)
    {
        const u8 *q = faces[f];
        if (d3_backface(sx[q[0]], sy[q[0]], sx[q[1]], sy[q[1]], sx[q[2]], sy[q[2]]))
            continue;

        // dithered face skin, then carve the carpet out of it
        bmp_fillTri(sx[q[0]], sy[q[0]], sx[q[1]], sy[q[1]], sx[q[2]], sy[q[2]], 2 + f, 1);
        bmp_fillTri(sx[q[0]], sy[q[0]], sx[q[2]], sy[q[2]], sx[q[3]], sy[q[3]], 1, 2 + f);

        s16 ffx[4] = { sx[q[0]], sx[q[1]], sx[q[2]], sx[q[3]] };
        s16 ffy[4] = { sy[q[0]], sy[q[1]], sy[q[2]], sy[q[3]] };
        carpet(ffx, ffy, 2);

        for (u16 e = 0; e < 4; e++)
            bmp_lineSafe(ffx[e], ffy[e], ffx[(e + 1) & 3], ffy[(e + 1) & 3], 15);
    }
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
    PAL_setColor(16 + 6, VCOL(6, 2, 4));
    PAL_setColor(16 + 7, VCOL(3, 5, 7));
    PAL_setColor(16 + 15, VCOL(7, 7, 7));
    VDP_setBackgroundColor(16);

    snd_setMood(SND_CALM);
}

void fractal_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    if (t < 260)
    {
        // I. sierpinski bloom
        maxDepth = t / 90;
        if (maxDepth > 3) maxDepth = 3;
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
    }
    else if (t < 480)
    {
        // II. the fractal tree grows out of the last triangle
        u16 lt = t - 260;
        u8 grow = 1 + (lt / 35);
        if (grow > 7) grow = 7;
        s16 len = 28 + (lt >> 3);
        if (len > 44) len = 44;
        frac_branch(128, 150, 64, len, 7, grow, t);
        // mirrored roots, fainter
        frac_branch(128, 150, 192, len >> 1, 5, grow > 2 ? grow - 2 : 0, t);
    }
    else
    {
        // III. the menger cube - complexity made solid
        mengerCube(t - 480);
    }

    // phase-change flash
    if (t == 260 || t == 480) fx_fillPal(16, 1, VCOL(3, 3, 4));
    else if (t == 264 || t == 484) PAL_setColor(16, 0x0000);

    BMP_flip(1);

    if ((t & 3) == 0)
    {
        PAL_setColor(16 + 2, fx_hue(t & 255, 5));
        PAL_setColor(16 + 3, fx_hue((t + 60) & 255, 7));
        PAL_setColor(16 + 5, fx_hue((t + 140) & 255, 6));
    }
}

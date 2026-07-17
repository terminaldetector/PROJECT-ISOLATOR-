#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"

// scene 12, the epilogue: a colored 3D flower falls slowly through the
// frame, shedding petals, gradients shifting - until a plain sunset
// takes the screen. texhnolyze mood. full palette burn.

#define NPETAL   8
#define PETAL_R  34
#define HORIZON  118

typedef struct
{
    u16 attached;
    s16 x8, y8;         // screen pos, 13.3 fixed
    s16 vy;
    u16 phase;          // personal sway phase
} Petal;

static Petal pet[NPETAL];
static u16 detachOrder[NPETAL];

// project through d3 then shift vertically - the flower drifts down the frame
static void fl_line(const V3 *a, const V3 *b, u8 col, s16 dy)
{
    s16 x1, y1, x2, y2;
    if (d3_project(a, &x1, &y1) && d3_project(b, &x2, &y2))
        bmp_lineSafe(x1, y1 + dy, x2, y2 + dy, col);
}

static void drawPetal3D(u16 ang, s16 droop, u8 col, s16 dy)
{
    V3 base = { 0, 0, 0 };
    V3 tip  = { (PETAL_R * COS(ang)) >> 8, -droop, (PETAL_R * SIN(ang)) >> 8 };
    V3 s1   = { ((PETAL_R >> 1) * COS(ang + 14)) >> 8, -(droop >> 2),
                ((PETAL_R >> 1) * SIN(ang + 14)) >> 8 };
    V3 s2   = { ((PETAL_R >> 1) * COS(ang - 14)) >> 8, -(droop >> 2),
                ((PETAL_R >> 1) * SIN(ang - 14)) >> 8 };
    fl_line(&base, &s1, col, dy);
    fl_line(&s1, &tip, col, dy);
    fl_line(&tip, &s2, col, dy);
    fl_line(&s2, &base, col, dy);
    fl_line(&base, &tip, col, dy);
}

// a detached petal flutters down in 2D
static void drawPetal2D(const Petal *p, u16 t, u16 i, u8 col)
{
    s16 cx = (p->x8 >> 3) + (SIN((t << 1) + p->phase) >> 4);
    s16 cy = p->y8 >> 3;
    u16 rot = (t << 1) + (i << 4);
    s16 lx = (10 * COS(rot)) >> 8, ly = (10 * SIN(rot)) >> 8;
    s16 sx = (5 * COS(rot + 64)) >> 8, sy = (5 * SIN(rot + 64)) >> 8;
    bmp_lineSafe(cx + lx, cy + ly, cx + sx, cy + sy, col);
    bmp_lineSafe(cx + sx, cy + sy, cx - lx, cy - ly, col);
    bmp_lineSafe(cx - lx, cy - ly, cx - sx, cy - sy, col);
    bmp_lineSafe(cx - sx, cy - sy, cx + lx, cy + ly, col);
}

static u16 scaleCol(u16 c, u16 bf)
{
    u16 r = ((c >> 1) & 7) * bf / 7;
    u16 g = ((c >> 5) & 7) * bf / 7;
    u16 b = ((c >> 9) & 7) * bf / 7;
    return VCOL(r, g, b);
}

void flower_init(void)
{
    demo_enterBMP();

    for (u16 i = 0; i < NPETAL; i++)
    {
        pet[i].attached = TRUE;
        detachOrder[i] = (i * 5) & 7;           // scrambled shedding order
        pet[i].phase = rnd() & 255;
    }

    fx_allBlack();
    snd_setMood(SND_AGONY);
}

void flower_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    // global brightness: full until the last stretch, then dies with the sun
    u16 bf = 7;
    if (t > 1000) bf = 7 - ((t - 1000) / 22);
    if (bf > 7) bf = 0;

    // ---- sunset backdrop ----
    if (t > 380)
    {
        u16 rise = (t - 380) >> 2;
        if (rise > 60) rise = 60;

        // sky bands above the horizon, sparse scanlines
        for (s16 y = HORIZON - rise; y < HORIZON; y += 4)
        {
            u8 band = 10 + ((HORIZON - y) >> 4);
            if (band > 13) band = 13;
            bmp_lineSafe(0, y, 255, y, band);
        }
        // horizon
        bmp_lineSafe(0, HORIZON, 255, HORIZON, 12);
        // dark water below, thinning reflections
        for (s16 y = HORIZON + 4; y < 160; y += 8)
            bmp_lineSafe(64 - (y - HORIZON), y, 192 + (y - HORIZON), y, 1);

        // the sun sinks
        if (t > 450)
        {
            s16 sunCy = 78 + ((t - 450) >> 3);
            s16 r = 30;
            for (s16 yy = sunCy - r; yy < HORIZON; yy += 2)
            {
                s16 d = yy - sunCy;
                if (d < 0) d = -d;
                if (d > r) continue;
                // integer half-width of the disc at this scanline
                s16 v = r * r - d * d;
                s16 w = r;
                while (w * w > v) w--;
                bmp_lineSafe(128 - w, yy, 128 + w, yy, 14);
            }
        }
    }

    // ---- the flower ----
    s16 cy = -30 + (t >> 1);
    if (cy > 126) cy = 126;
    s16 dy = cy - 80;

    u16 spin = t + (SIN(t >> 1) >> 5);
    d3_setCamera(24, spin, 215);

    s16 wilt = 6 + (t >> 6);                    // petals droop as it falls

    // stem trails above the falling head
    V3 sTop = { SIN(t) >> 4, 46, 0 };
    V3 sBase = { 0, 4, 0 };
    fl_line(&sBase, &sTop, 2 + 7, dy);          // dark green-ish slot (color 9)
    V3 leaf = { 12, 30, 4 };
    fl_line(&sTop, &leaf, 9, dy);

    for (u16 i = 0; i < NPETAL; i++)
    {
        u16 idx = detachOrder[i];
        u8 col = 2 + idx;

        // shedding schedule: one petal lets go every ~70 frames after t=260
        if (pet[idx].attached && t > 260 + i * 70)
        {
            pet[idx].attached = FALSE;
            pet[idx].x8 = (128 + (SIN((idx << 5) + spin) >> 3)) << 3;
            pet[idx].y8 = cy << 3;
            pet[idx].vy = 4 + (rnd() & 7);
        }

        if (pet[idx].attached)
            drawPetal3D(idx << 5, wilt, col, dy);
        else if ((pet[idx].y8 >> 3) < 170)
        {
            pet[idx].y8 += pet[idx].vy;
            drawPetal2D(&pet[idx], t, idx, col);
        }
    }

    // flower heart
    for (u16 i = 0; i < 8; i++)
    {
        u16 a1 = i << 5, a2 = (i + 1) << 5;
        bmp_lineSafe(128 + ((6 * COS(a1)) >> 8), 80 + dy - ((6 * SIN(a1)) >> 8),
                     128 + ((6 * COS(a2)) >> 8), 80 + dy - ((6 * SIN(a2)) >> 8), 15);
    }

    BMP_flip(1);

    // ---- full-palette gradient work, morphing every frame ----
    // petals: the whole RGB wheel, slowly drifting
    for (u16 i = 0; i < NPETAL; i++)
        PAL_setColor(16 + 2 + i, scaleCol(fx_hue((t >> 1) + (i << 5), 6), bf));

    // sky: cold night -> burning dusk -> ash
    u16 warm = (t < 400) ? 0 : ((t - 400) >> 4);
    if (warm > 32) warm = 32;
    PAL_setColor(16 + 0, scaleCol(VCOL(warm >> 4, 0, 1 + (warm >> 5)), bf));
    PAL_setColor(16 + 1, scaleCol(VCOL(1 + (warm >> 4), 0, 2), bf));
    PAL_setColor(16 + 10, scaleCol(VCOL(2 + (warm >> 3), 1, 4), bf));
    PAL_setColor(16 + 11, scaleCol(VCOL(3 + (warm >> 3), 1, 3), bf));
    PAL_setColor(16 + 12, scaleCol(VCOL(4 + (warm >> 3), 2, 2), bf));
    PAL_setColor(16 + 13, scaleCol(VCOL(5 + (warm >> 4), 3, 1), bf));

    // the sun: yellow -> blood red as it dies
    u16 sg = 6 - (t > 700 ? ((t - 700) / 90) : 0);
    if (sg > 6) sg = 0;
    PAL_setColor(16 + 14, scaleCol(VCOL(7, sg, sg >> 2), bf));

    PAL_setColor(16 + 9, scaleCol(VCOL(1, 4, 2), bf));
    PAL_setColor(16 + 15, scaleCol(VCOL(7, 7, 7), bf));
    VDP_setBackgroundColor(16);
}

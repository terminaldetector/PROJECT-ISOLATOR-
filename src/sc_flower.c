#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "fxhint.h"
#include "draw3d.h"
#include "sound.h"
#include "seq.h"

// scene 12, the showcase: a colored 3D flower falls from the sky the way
// Ran's flower falls in Texhnolyze - a slow pendulum drift, not a drop.
// dithered filled petals, a copper-gradient sunset, and every gesture
// locked to the music: petals let go on the downbeat, the sun pulses
// with the kick.

#define NPETAL   8
#define PETAL_R  36
#define HORIZON  118

typedef struct
{
    u16 attached;
    s16 x8, y8;
    s16 vy;
    u16 phase;
    s16 restX;          // where it finally lies
} Petal;

static Petal pet[NPETAL];
static u16 detachOrder[NPETAL];
static u16 shed;                // how many petals have been shed
static u16 sunPulse;
static s16 fallY;               // flower head screen y, 8.8

static void fl_project(const V3 *v, s16 *sx, s16 *sy, s16 dx, s16 dy, u16 *ok)
{
    if (!d3_project(v, sx, sy)) { *ok = FALSE; return; }
    *sx += dx;
    *sy += dy;
}

// petal as two dithered triangles + bright rim
static void drawPetal3D(u16 ang, s16 droop, u8 col, s16 dx, s16 dy)
{
    V3 base = { 0, 0, 0 };
    V3 tip  = { (PETAL_R * COS(ang)) >> 8, -droop, (PETAL_R * SIN(ang)) >> 8 };
    V3 s1   = { ((PETAL_R * 5 / 8) * COS(ang + 16)) >> 8, -(droop >> 2),
                ((PETAL_R * 5 / 8) * SIN(ang + 16)) >> 8 };
    V3 s2   = { ((PETAL_R * 5 / 8) * COS(ang - 16)) >> 8, -(droop >> 2),
                ((PETAL_R * 5 / 8) * SIN(ang - 16)) >> 8 };

    s16 bx, by, tx, ty, ax, ay, cx2, cy2;
    u16 ok = TRUE;
    fl_project(&base, &bx, &by, dx, dy, &ok);
    fl_project(&tip, &tx, &ty, dx, dy, &ok);
    fl_project(&s1, &ax, &ay, dx, dy, &ok);
    fl_project(&s2, &cx2, &cy2, dx, dy, &ok);
    if (!ok) return;

    // dithered body: inner shade blends toward the darker twin color
    bmp_fillTri(bx, by, ax, ay, tx, ty, col, col - 1);
    bmp_fillTri(bx, by, tx, ty, cx2, cy2, col - 1, col);
    // luminous rim
    bmp_lineSafe(ax, ay, tx, ty, 15);
    bmp_lineSafe(tx, ty, cx2, cy2, 15);
}

static void drawPetal2D(const Petal *p, u16 t, u16 i, u8 col)
{
    s16 cx = (p->x8 >> 3) + (SIN((t << 1) + p->phase) >> 4);
    s16 cy = p->y8 >> 3;
    u16 rot = (t << 1) + (i << 4);
    s16 lx = (11 * COS(rot)) >> 8, ly = (11 * SIN(rot)) >> 8;
    s16 sx = (5 * COS(rot + 64)) >> 8, sy = (5 * SIN(rot + 64)) >> 8;
    bmp_fillTri(cx + lx, cy + ly, cx + sx, cy + sy, cx - lx, cy - ly, col, col - 1);
    bmp_fillTri(cx - lx, cy - ly, cx - sx, cy - sy, cx + lx, cy + ly, col - 1, col);
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
        detachOrder[i] = (i * 5) & 7;
        pet[i].phase = rnd() & 255;
        pet[i].restX = 0;
    }
    shed = 0;
    sunPulse = 0;
    fallY = -(30 << 8);

    fx_allBlack();
    VDP_setBackgroundColor(16);

    snd_setMood(SND_AGONY);
}

void flower_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    u16 bf = 7;
    if (t > 1000) bf = 7 - ((t - 1000) / 22);
    if (bf > 7) bf = 0;

    // ---- Ran's fall: slow vertical drift with a pendulum swing ----
    // the flower sinks at ~8 px/sec, swaying; rotation follows the sway
    fallY += 40 + (SIN(t) >> 4);
    s16 cy = (fallY >> 8);
    if (cy > 122) cy = 122;
    s16 swing = SIN(t >> 1) >> 2;       // +-16 px pendulum
    s16 dx = swing;
    s16 dy = cy - 80;

    u16 spin = (t >> 1) + (swing >> 1); // tilt into the swing
    d3_setCamera(28, spin, 210);

    // ---- sky drawn into the bitmap: night -> burning dusk (indices morph
    //      each frame, so the whole sky glows with the sunset) ----
    {
        static const u8 sky[6] = { 1, 1, 10, 11, 12, 13 };
        bmp_vgradRamp(0, HORIZON, sky, 6);
    }

    // ---- sunset backdrop ----
    if (t > 380)
    {
        u16 rise = (t - 380) >> 2;
        if (rise > 60) rise = 60;

        bmp_lineSafe(0, HORIZON, 255, HORIZON, 12);
        // dithered water: paired spans below the horizon
        for (s16 y = HORIZON + 2; y < 160; y += 4)
            bmp_hspan(60 - (y - HORIZON), 196 + (y - HORIZON), y, BCOL2(1, 0));

        if (t > 450)
        {
            s16 sunCy = 76 + ((t - 450) >> 3);
            s16 r = 30 + ((sunPulse > 0) ? 2 : 0);
            if (sunPulse) sunPulse--;
            for (s16 yy = sunCy - r; yy < HORIZON; yy += 1)
            {
                s16 d = yy - sunCy;
                if (d < 0) d = -d;
                if (d > r) continue;
                s16 v = r * r - d * d;
                s16 w = r;
                while (w * w > v) w--;
                // dithered limb, solid core
                bmp_hspan(128 - w, 128 + w, yy, (yy & 1) ? BCOL(14) : BCOL2(14, 13));
            }
        }
    }
    else
    {
        // early: sparse stars above, mirrored below faintly
        for (u16 i = 0; i < 3; i++)
            BMP_setPixel(rnd() & 255, rnd_range(100), BCOL(4));
    }

    // ---- the flower ----
    s16 wilt = 6 + (t >> 6);

    V3 sTop = { SIN(t) >> 4, 46, 0 };
    V3 sBase = { 0, 4, 0 };
    s16 x1, y1, x2, y2;
    u16 ok = TRUE;
    fl_project(&sBase, &x1, &y1, dx, dy, &ok);
    fl_project(&sTop, &x2, &y2, dx, dy, &ok);
    if (ok)
    {
        bmp_lineSafe(x1, y1, x2, y2, 9);
        bmp_lineSafe(x1 + 1, y1, x2 + 1, y2, 9);
    }

    // shedding: one petal per downbeat once the fall is underway
    if (seq_isDownbeat() && t > 260 && shed < NPETAL)
    {
        u16 idx = detachOrder[shed];
        pet[idx].attached = FALSE;
        pet[idx].x8 = (128 + dx + (SIN((idx << 5) + spin) >> 3)) << 3;
        pet[idx].y8 = cy << 3;
        pet[idx].vy = 3 + (rnd() & 3);
        pet[idx].restX = 30 + rnd_range(196);
        shed++;
    }

    for (u16 i = 0; i < NPETAL; i++)
    {
        u8 col = 2 + i;
        if (pet[i].attached)
            drawPetal3D(i << 5, wilt, col, dx, dy);
        else
        {
            s16 py = pet[i].y8 >> 3;
            if (py < 150)
            {
                pet[i].y8 += pet[i].vy;
                drawPetal2D(&pet[i], t, i, col);
            }
            else
                // at rest on the water, a dim shard
                bmp_hspan(pet[i].restX - 4, pet[i].restX + 4, 151 + (i & 3),
                          BCOL2(col - 1, 0));
        }
    }

    // flower heart - kick makes it flare
    u8 heart = seq_isKick() ? 15 : 14;
    for (u16 i = 0; i < 8; i++)
    {
        u16 a1 = i << 5, a2 = (i + 1) << 5;
        bmp_lineSafe(128 + dx + ((6 * COS(a1)) >> 8), 80 + dy - ((6 * SIN(a1)) >> 8),
                     128 + dx + ((6 * COS(a2)) >> 8), 80 + dy - ((6 * SIN(a2)) >> 8), heart);
    }
    if (seq_isKick()) sunPulse = 4;

    BMP_flip(1);

    // ---- sky ramp palette (indices 1,10,11,12,13): night -> burning dusk,
    //      morphing every frame; the in-bitmap gradient reads these ----
    u16 warm = (t < 380) ? 0 : ((t - 380) >> 4);
    if (warm > 40) warm = 40;
    PAL_setColor(16 + 1,  scaleCol(VCOL(warm > 24 ? 1 : 0, 0, 2), bf));
    PAL_setColor(16 + 10, scaleCol(VCOL(1 + (warm >> 4), 0, 2), bf));
    PAL_setColor(16 + 11, scaleCol(VCOL(3 + (warm >> 3), 1, 1), bf));
    PAL_setColor(16 + 12, scaleCol(VCOL(5 + (warm >> 4), 2, 1), bf));
    PAL_setColor(16 + 13, scaleCol(VCOL(7, 3 + (warm >> 4), 0), bf));

    // petals ride the full RGB wheel
    for (u16 i = 0; i < NPETAL; i++)
        PAL_setColor(16 + 2 + i, scaleCol(fx_hue((t >> 1) + (i << 5), 6), bf));

    PAL_setColor(16 + 9, scaleCol(VCOL(1, 4, 2), bf));
    u16 sg = 6 - (t > 700 ? ((t - 700) / 90) : 0);
    if (sg > 6) sg = 0;
    PAL_setColor(16 + 14, scaleCol(VCOL(7, sg, sg >> 2), bf));
    PAL_setColor(16 + 15, scaleCol(VCOL(7, 7, 7), bf));
}

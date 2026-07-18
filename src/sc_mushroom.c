#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "fxhint.h"
#include "draw3d.h"
#include "sound.h"
#include "seq.h"

// scene 9: procedural nuclear bloom - a detailed, Gunstar-flavoured blast.
// irradiated copper sky, a devastated city silhouette on the horizon, a
// billowing cauliflower cloud built from stacked dithered lobes over a
// full fire-ramp palette, screen shake and debris on the beat.

#define GY   152
#define CX0  128

#define PH_WAIT  0
#define PH_FLASH 1
#define PH_CLOUD 2

static u8  phase;
static u16 detFrame;
static u16 flashFrame;

// skyline: fixed silhouette of a ruined city, rim-lit by the blast
static u8  skyH[24];

// rising smoke lobes carry their own drift
#define NLOBE 10
static s16 lobeSeed[NLOBE];

void mushroom_init(void)
{
    demo_enterBMP();
    fx_allBlack();
    phase = PH_WAIT;
    detFrame = 0;
    flashFrame = 0;

    rnd_seed(0x4E42);
    for (u16 i = 0; i < 24; i++)
        skyH[i] = 6 + rnd_range(20);
    for (u16 i = 0; i < NLOBE; i++)
        lobeSeed[i] = rnd() & 255;

    snd_setMood(SND_HARD);
}

// the pretty plasma sphere: a FIXED number of concentric dithered rainbow
// bands from R inward, regardless of how large R grows. this keeps the
// draw cost bounded even once the flash has smeared across the whole
// screen - stacking a ring per unit of radius would make the frame cost
// grow with R^2, which is exactly the kind of slowdown we just fixed
// elsewhere with isqrt32.
#define FLASH_RINGS 8
static void plasmaBurst(s16 cx, s16 cy, s16 R, u16 fr)
{
    for (u16 k = 0; k < FLASH_RINGS; k++)
    {
        s16 rr = R - (s16)(((s32) R * k) / FLASH_RINGS);
        if (rr <= 0) break;
        u16 ph = (rr * 5) + (fr << 3);
        u8 h1 = 1 + (ph % 14);
        u8 h2 = 1 + ((ph + 4) % 14);
        bmp_disc(cx, cy, rr, h1, h2);
    }
    bmp_disc(cx, cy, 3 + (fr >> 1), 15, 14);

    // cracks of light tearing outward as it expands
    for (u16 i = 0; i < 10; i++)
    {
        u16 a = ((i * 256) / 10) + (fr << 2);
        s16 len = R + 10 + fr * 2;
        bmp_lineSafe(cx, cy, cx + ((len * COS(a)) >> 8), cy - ((len * SIN(a)) >> 8), 14);
    }
}

// the fire ramp: smoke -> maroon -> red -> orange -> gold -> yellow -> white,
// cooling as the fireball ages
static void heatPalette(u16 age)
{
    u16 cool = age >> 6;                          // 0..~10 over the scene
    static const u16 base[16][3] =
    {
        { 0, 0, 1 },   // 0  night sky (copper overrides most of it)
        { 1, 1, 1 },   // 1  dark smoke
        { 2, 2, 2 },   // 2  smoke
        { 3, 1, 1 },   // 3  ember maroon
        { 4, 1, 0 },   // 4  dark red
        { 5, 1, 0 },   // 5  red
        { 6, 2, 0 },   // 6  red-orange
        { 7, 3, 0 },   // 7  orange
        { 7, 4, 0 },   // 8  amber
        { 7, 5, 0 },   // 9  gold
        { 7, 6, 1 },   // 10 warm yellow
        { 7, 7, 2 },   // 11 yellow
        { 7, 7, 4 },   // 12 pale yellow
        { 7, 7, 6 },   // 13 cream
        { 7, 7, 7 },   // 14 white
        { 7, 7, 7 },   // 15 white core
    };
    for (u16 i = 1; i < 16; i++)
    {
        s16 r = base[i][0] - (cool >> 1);
        s16 g = base[i][1] - cool;
        s16 b = base[i][2];
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        PAL_setColor(16 + i, VCOL(r, g, b));
    }
    PAL_setColor(16, VCOL(0, 0, 0));
}

// irradiated sky, drawn INTO the bitmap (no HInt copper - that hangs the
// software-bitmap flip): dark at the top burning to amber at the horizon,
// hotter right after the flash
static void skyGradient(u16 age, u16 kick)
{
    // ramps use the fire palette itself, so no extra CRAM is spent
    static const u8 calm[8] = { 1, 1, 3, 4, 5, 6, 7, 8 };
    static const u8 hot[8]  = { 3, 4, 5, 6, 7, 8, 9, 11 };
    bmp_vgradRamp(0, GY, (age < 90 || kick) ? hot : calm, 8);
}

void mushroom_update(u16 t)
{
    if (phase == PH_WAIT)
    {
        // building anticipation: a faint trembling point of light at
        // ground zero, waiting for the music to drop
        BMP_waitWhileFlipRequestPending();
        BMP_clear();
        if (t > 4)
        {
            u8 glow = 2 + (t & 3);
            bmp_disc(CX0, GY - 10, 2 + (t >> 5), glow, glow - 1);
        }
        BMP_flip(1);

        if ((t > 8 && seq_isDownbeat()) || t > 120)
        {
            phase = PH_FLASH;
            flashFrame = 0;
            heatPalette(0);
            VDP_setBackgroundColor(16);
            snd_boom();
        }
        return;
    }

    if (phase == PH_FLASH)
    {
        // the pretty sphere is born, then smears outward until it has
        // swallowed the whole screen - THEN the mushroom grows from it
        BMP_waitWhileFlipRequestPending();
        BMP_clear();

        u16 fr = flashFrame++;
        s16 R = 4 + (s16)(((s32) fr * fr) / 3);
        plasmaBurst(CX0, GY - 10, R, fr);

        BMP_flip(1);

        if (R > 220 || fr > 26)
        {
            phase = PH_CLOUD;
            detFrame = 0;
            fx_allWhite();
        }
        return;
    }

    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    u16 age = detFrame++;

    // screen shake from the shock, decaying
    s16 shake = 0;
    if (age < 40) shake = (SIN(age * 48) * (40 - age)) >> 10;
    s16 cx = CX0 + shake;

    // irradiated sky behind everything
    skyGradient(age, seq_isKick());

    // growth curves
    s16 stemH = age;
    if (stemH > 96) stemH = 96;
    s16 sw = 7 + (age >> 3);
    if (sw > 18) sw = 18;
    s16 cr = 14 + (age >> 1);
    if (cr > 60) cr = 60;
    s16 capY = GY - stemH;

    // ---- devastated city silhouette on the horizon, rim-lit ----
    for (u16 i = 0; i < 24; i++)
    {
        s16 bx = i * 11 - 4;
        s16 bh = skyH[i];
        // buildings closer to ground zero are taller rubble
        s16 dist = (i > 11) ? (i - 11) : (11 - i);
        s16 h = bh - (dist < 6 ? (6 - dist) : 0);
        if (h < 3) h = 3;
        bmp_hspan(bx + shake, bx + 10 + shake, GY - 1, BCOL(2));
        for (s16 yy = 0; yy < h; yy++)
            bmp_hspan(bx + shake, bx + 9 + shake, GY - 2 - yy,
                      (yy & 1) ? BCOL2(1, 3) : BCOL2(3, 1));
        // blast-lit top edge
        bmp_hspan(bx + shake, bx + 9 + shake, GY - 2 - h, BCOL(7));
    }

    // ---- ground: scorched, rubble-flecked ----
    for (s16 yy = 0; yy < 160 - GY; yy++)
        bmp_hspan(0, 255, GY + yy, (yy & 1) ? BCOL2(3, 1) : BCOL2(1, 4));

    // ---- stem: turbulent fire column, widening trumpet toward the cap ----
    for (s16 yy = GY; yy > capY + (cr >> 2); yy--)
    {
        s16 f = (GY - yy);
        s16 wHere = sw + (f * sw) / (stemH + 1);    // flares upward
        s16 wob = (SIN((age << 2) + (yy << 2)) >> 5);
        s16 lx = cx - wHere + wob, rx = cx + wHere + wob;
        // dithered fire: bright core, cooler edges
        bmp_hspan(lx, rx, yy, (yy & 1) ? BCOL2(6, 9) : BCOL2(9, 6));
        bmp_hspan(cx - (wHere >> 1) + wob, cx + (wHere >> 1) + wob, yy,
                  (yy & 1) ? BCOL2(12, 11) : BCOL2(11, 12));
    }

    // ---- billowing cauliflower cap: stacked dithered lobes ----
    // a big central bloom plus a ring of rolling lobes around the crown
    bmp_ellipse(cx, capY, cr, (cr * 3) / 4, 8, 6);
    for (u16 i = 0; i < NLOBE; i++)
    {
        u16 a = (i * 256) / NLOBE;
        s16 bulge = (SIN((age << 1) + lobeSeed[i]) >> 5);
        s16 lr = (cr * 5) / 12 + bulge;
        s16 lx = cx + ((( (cr * 3) / 4) * COS(a)) >> 8);
        s16 ly = capY - (((cr / 2) * SIN(a)) >> 8);
        // upper lobes hotter, lower/outer lobes cooler smoke
        u8 hot = (SIN(a) > 40) ? 11 : 8;
        u8 cool = (SIN(a) > 40) ? 9 : 5;
        bmp_disc(lx, ly, lr, hot, cool);
    }
    // white-hot heart of the cloud
    bmp_disc(cx, capY, cr >> 2, 15, 13);
    // cap underside shadow
    bmp_ellipse(cx, capY + (cr >> 2), (cr * 5) / 6, cr >> 2, 4, 3);

    // ---- expanding shock rings, one per bar ----
    u16 ringBase = (seq_bar() & 3) * 22;
    s16 srx = (age << 1) + ringBase;
    s16 sry = (age / 3) + (ringBase >> 3);
    if (srx < 280)
        for (u16 i = 0; i < 24; i++)
        {
            u16 a1 = (i * 256) / 24, a2 = ((i + 1) * 256) / 24;
            bmp_lineSafe(cx + ((srx * COS(a1)) >> 8), GY - ((sry * SIN(a1)) >> 8),
                         cx + ((srx * COS(a2)) >> 8), GY - ((sry * SIN(a2)) >> 8),
                         (i & 1) ? 13 : 7);
        }

    // ---- debris: chunks arcing up with fiery trails ----
    for (u16 i = 0; i < 10; i++)
    {
        u16 ph = (age * 3 + i * 40) & 255;
        s16 dx = ((s16)(lobeSeed[i % NLOBE] - 128) * ph) >> 9;
        s16 dy = -((ph * (256 - ph)) >> 7);          // parabola up then down
        s16 ex = cx + dx, ey = capY + 20 + dy;
        bmp_lineSafe(ex, ey, ex - (dx >> 3), ey + 4, 8);
        BMP_setPixel(ex, ey, BCOL(13));
    }

    // ---- lightning: jagged cracks of light off the fireball, more
    //      dynamism as the cloud churns, tapering off as it cools ----
    if (age < 400 && ((rnd() & 5) == 0 || seq_isKick()))
    {
        s16 x0 = cx, y0 = capY + (s16) rnd_range(cr > 4 ? cr : 4);
        u16 a = rnd() & 255;
        s16 len = 24 + (rnd() & 63);
        s16 x1 = x0 + ((len * COS(a)) >> 8);
        s16 y1 = y0 - ((len * SIN(a)) >> 8);
        s16 mx = ((x0 + x1) >> 1) + (s16)(rnd() & 15) - 8;
        s16 my = ((y0 + y1) >> 1) + (s16)(rnd() & 15) - 8;
        bmp_lineSafe(x0, y0, mx, my, 15);
        bmp_lineSafe(mx, my, x1, y1, 11);
    }
    // sparks around the fireball
    for (u16 i = 0; i < 8; i++)
        BMP_setPixel(cx + (s16)(rnd() & 127) - 64, capY + (s16)(rnd() & 63) - 32,
                     BCOL(11 + (rnd() & 4)));

    BMP_flip(1);

    // the fire ramp cools as the fireball ages
    if ((age & 15) == 0) heatPalette(age);

    // aftershocks ride the snare
    if (seq_isSnare() && age > 120) snd_boom();
}

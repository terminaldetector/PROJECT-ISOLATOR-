#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"
#include "seq.h"

// scene 5: the fractal folds into a tunnel - rings fly at the camera,
// a living iridescent plasma sphere breathing at the vanishing point

#define RINGS     8
#define RING_SEG  8
#define RING_R    120

// the RGBV plasma core: concentric rainbow bands that cycle and blend,
// a white-hot heart, and rays radiating outward that pulse on the beat
static void plasmaSphere(s16 cx, s16 cy, s16 R, u16 t, u16 kick)
{
    // radial spectrum from the rim inward, dithered between neighbouring hues
    for (s16 rr = R; rr > 0; rr -= 2)
    {
        u16 phase = (rr * 4) + (t << 1);
        u8 h  = 1 + (phase % 14);
        u8 h2 = 1 + ((phase + 4) % 14);
        bmp_disc(cx, cy, rr, h, h2);
    }
    // white-hot heart, flaring on the kick
    bmp_disc(cx, cy, 3 + (kick ? 3 : 0), 15, 14);

    // radiating rays, length pulsing with the beat
    s16 rayLen = R + 12 + (kick ? 16 : 0) + (SIN(t << 2) >> 4);
    for (u16 i = 0; i < 12; i++)
    {
        u16 a = (i * 256) / 12 + (t << 1);
        u8 col = 1 + ((i * 3 + (t >> 2)) % 14);
        s16 x0 = cx + ((R * COS(a)) >> 8), y0 = cy - ((R * SIN(a)) >> 8);
        s16 x1 = cx + ((rayLen * COS(a)) >> 8), y1 = cy - ((rayLen * SIN(a)) >> 8);
        bmp_lineSafe(x0, y0, x1, y1, col);
        BMP_setPixel(x1, y1, BCOL(15));
    }
}

void tunnel_init(void)
{
    demo_enterBMP();

    PAL_setColor(16 + 0, 0x0000);
    for (u16 i = 1; i < 15; i++)
        PAL_setColor(16 + i, fx_hue(i * 17, 3 + (i >> 2)));
    PAL_setColor(16 + 15, VCOL(7, 7, 7));
    VDP_setBackgroundColor(16);

    snd_setMood(SND_DRIVE);
}

void tunnel_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    // tunnel center swims around
    s16 cx = 128 + (SIN(t) >> 3);
    s16 cy = 80 + (SIN((t << 1) + 60) >> 4);

    s16 px[RING_SEG + 1], py[RING_SEG + 1];
    s16 qx[RING_SEG + 1], qy[RING_SEG + 1];
    u16 havePrev = FALSE;

    for (u16 i = 0; i < RINGS; i++)
    {
        // ring depth cycles toward the viewer
        u16 z = 300 - (((i << 6) + (t << 2)) & 511);
        if (z < 24 || z > 300)
        {
            havePrev = FALSE;
            continue;
        }
        u16 rot = t + (i << 3);
        u8 col = 1 + ((i + (t >> 3)) % 14);

        // the whole tube bends - distant rings swing away from the center
        s16 bend = ((s32) SIN((t << 1) + z) * (300 - z)) >> 11;
        s16 rcx = cx + bend;

        for (u16 s = 0; s <= RING_SEG; s++)
        {
            u16 a = rot + (s * 256) / RING_SEG;
            px[s] = rcx + (((RING_R * COS(a)) >> 8) * 160) / z;
            py[s] = cy - (((RING_R * SIN(a)) >> 8) * 160) / z;
        }
        for (u16 s = 0; s < RING_SEG; s++)
            bmp_lineSafe(px[s], py[s], px[s + 1], py[s + 1], col);

        if (havePrev)
        {
            // near rings: checkered dithered wall panels
            if (z < 130)
            {
                for (u16 s = 0; s < RING_SEG; s++)
                {
                    if (((s + (t >> 4)) & 1) == 0) continue;
                    bmp_fillTri(px[s], py[s], px[s + 1], py[s + 1],
                                qx[s + 1], qy[s + 1], col, 0);
                    bmp_fillTri(px[s], py[s], qx[s + 1], qy[s + 1],
                                qx[s], qy[s], col, 0);
                }
            }
            for (u16 s = 0; s < RING_SEG; s += 2)
                bmp_lineSafe(px[s], py[s], qx[s], qy[s], col);
        }

        for (u16 s = 0; s <= RING_SEG; s++)
        {
            qx[s] = px[s];
            qy[s] = py[s];
        }
        havePrev = TRUE;
    }

    // the living plasma sphere at the vanishing point - breathing radius
    s16 R = 20 + (SIN(t << 2) >> 5) + (SIN(t) >> 6);
    plasmaSphere(cx, cy, R, t, seq_isKick());

    // speed streaks from the sphere outward
    for (u16 i = 0; i < 4; i++)
    {
        u16 a = rnd() & 255;
        s16 r1 = R + 4 + (rnd() & 31);
        s16 r2 = r1 + 30 + (rnd() & 31);
        bmp_lineSafe(cx + ((r1 * COS(a)) >> 8), cy - ((r1 * SIN(a)) >> 8),
                     cx + ((r2 * COS(a)) >> 8), cy - ((r2 * SIN(a)) >> 8), 15);
    }

    BMP_flip(1);

    // hard palette cycling - the tunnel strobes
    if ((t & 1) == 0)
        for (u16 i = 1; i < 15; i++)
            PAL_setColor(16 + i, fx_hue((i * 17 + (t << 2)) & 255, 3 + (i >> 2)));
}

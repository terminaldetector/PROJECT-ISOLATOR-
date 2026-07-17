#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"

// scene 5: the fractal folds into a tunnel - rings fly at the camera

#define RINGS     8
#define RING_SEG  8
#define RING_R    120

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

        for (u16 s = 0; s <= RING_SEG; s++)
        {
            u16 a = rot + (s * 256) / RING_SEG;
            px[s] = cx + (((RING_R * COS(a)) >> 8) * 160) / z;
            py[s] = cy - (((RING_R * SIN(a)) >> 8) * 160) / z;
        }
        for (u16 s = 0; s < RING_SEG; s++)
            bmp_lineSafe(px[s], py[s], px[s + 1], py[s + 1], col);

        // spokes connect neighbouring rings
        if (havePrev)
            for (u16 s = 0; s < RING_SEG; s += 2)
                bmp_lineSafe(px[s], py[s], qx[s], qy[s], col);

        for (u16 s = 0; s <= RING_SEG; s++)
        {
            qx[s] = px[s];
            qy[s] = py[s];
        }
        havePrev = TRUE;
    }

    // speed streaks from the center
    for (u16 i = 0; i < 4; i++)
    {
        u16 a = rnd() & 255;
        s16 r1 = 20 + (rnd() & 31);
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

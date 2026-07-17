#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "fxhint.h"
#include "draw3d.h"
#include "sound.h"
#include "seq.h"

// scene 14: aleph. one letter and nothing else.
// the great diagonal vav with two yods - drawn stroke by stroke in
// thick calligraphic lines, breathing with the music.

// a thick line: three parallel passes
static void stroke(s16 x1, s16 y1, s16 x2, s16 y2, u8 col, u16 reveal)
{
    // reveal: 0..256 - how much of the stroke exists yet
    s16 ex = x1 + (((s32)(x2 - x1) * reveal) >> 8);
    s16 ey = y1 + (((s32)(y2 - y1) * reveal) >> 8);
    for (s16 o = -2; o <= 2; o++)
    {
        bmp_lineSafe(x1 + o, y1, ex + o, ey, col);
        bmp_lineSafe(x1, y1 + o, ex, ey + o, col);
    }
}

static u16 rv(u16 t, u16 from, u16 span)
{
    if (t < from) return 0;
    u16 r = ((t - from) << 8) / span;
    return (r > 256) ? 256 : r;
}

void aleph_init(void)
{
    demo_enterBMP();

    fx_allBlack();
    VDP_setBackgroundColor(16);
    copper_enable(16);
    copper_gradient3(VCOL(0, 0, 1), VCOL(1, 0, 2), VCOL(0, 0, 0), 8);

    PAL_setColor(16 + 10, VCOL(7, 6, 0));   // the letter, gold
    PAL_setColor(16 + 11, VCOL(5, 4, 0));
    PAL_setColor(16 + 14, VCOL(3, 3, 5));
    PAL_setColor(16 + 15, VCOL(7, 7, 7));

    snd_setMood(SND_MYTHIC);
}

void aleph_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    // the letter breathes: brightness rides the bar, flares on the kick
    u8 body = seq_isKick() ? 15 : 10;

    // א : the main diagonal (vav), upper-right yod, lower-left yod
    // centered around (128, 80), roughly 72px tall
    stroke(166, 44, 90, 116, body, rv(t, 40, 120));                 // the diagonal
    stroke(108, 46, 128, 78, body, rv(t, 180, 90));                 // upper yod arm
    stroke(108, 46, 100, 60, 11, rv(t, 240, 60));                   // its head
    stroke(148, 82, 128, 114, body, rv(t, 300, 90));                // lower yod arm
    stroke(148, 82, 156, 100, 11, rv(t, 360, 60));                  // its foot

    // a faint halo once the letter is whole
    if (t > 430)
    {
        s16 r = 58 + (SIN(t << 1) >> 6);
        for (u16 i = 0; i < 16; i++)
        {
            u16 a1 = i << 4, a2 = (i + 1) << 4;
            bmp_lineSafe(128 + ((r * COS(a1)) >> 8), 80 - (((r * SIN(a1)) >> 8) * 3) / 4,
                         128 + ((r * COS(a2)) >> 8), 80 - (((r * SIN(a2)) >> 8) * 3) / 4, 14);
        }
    }

    BMP_flip(1);

    // the void hums: deep indigo bands drifting with the bass
    if ((t & 7) == 0)
    {
        u16 pulse = (seq_step() < 4) ? 1 : 0;
        copper_gradient3(VCOL(0, 0, 1), VCOL(1 + pulse, 0, 2 + pulse), VCOL(0, 0, 0),
                         8 + (SIN(t >> 2) >> 6));
    }
    if (seq_isDownbeat()) PAL_setColor(16 + 10, VCOL(7, 7, 3));
    else if ((t & 15) == 8) PAL_setColor(16 + 10, VCOL(7, 6, 0));
}

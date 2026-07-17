#include "fxhint.h"
#include "util.h"

u16 copperColors[COPPER_BANDS];

static vu16 copIdx;
static u16 copperOn = FALSE;

static HINTERRUPT_CALLBACK copperHInt(void)
{
    // one CRAM write fits comfortably in hblank
    *((vu32 *) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32) 0);
    *((vu16 *) VDP_DATA_PORT) = copperColors[(copIdx < COPPER_BANDS - 1) ? ++copIdx : copIdx];
}

static void copperVInt(void)
{
    copIdx = 0;
    *((vu32 *) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32) 0);
    *((vu16 *) VDP_DATA_PORT) = copperColors[0];
}

void copper_enable(void)
{
    copIdx = 0;
    SYS_setHIntCallback(copperHInt);
    SYS_setVIntCallback(copperVInt);
    VDP_setHIntCounter(7);
    VDP_setHInterrupt(TRUE);
    copperOn = TRUE;
}

void copper_disable(void)
{
    if (!copperOn) return;
    VDP_setHInterrupt(FALSE);
    SYS_setHIntCallback(NULL);
    SYS_setVIntCallback(NULL);
    copperOn = FALSE;
}

static u16 mix(u16 c1, u16 c2, u16 t, u16 tmax)
{
    s16 r1 = (c1 >> 1) & 7, g1 = (c1 >> 5) & 7, b1 = (c1 >> 9) & 7;
    s16 r2 = (c2 >> 1) & 7, g2 = (c2 >> 5) & 7, b2 = (c2 >> 9) & 7;
    return VCOL(r1 + ((r2 - r1) * (s16) t) / (s16) tmax,
                g1 + ((g2 - g1) * (s16) t) / (s16) tmax,
                b1 + ((b2 - b1) * (s16) t) / (s16) tmax);
}

void copper_gradient(u16 c1, u16 c2)
{
    for (u16 i = 0; i < COPPER_BANDS; i++)
        copperColors[i] = mix(c1, c2, i, COPPER_BANDS - 1);
}

void copper_gradient3(u16 c1, u16 c2, u16 c3, u16 m)
{
    if (m == 0) m = 1;
    if (m >= COPPER_BANDS - 1) m = COPPER_BANDS - 2;
    for (u16 i = 0; i <= m; i++)
        copperColors[i] = mix(c1, c2, i, m);
    for (u16 i = m; i < COPPER_BANDS; i++)
        copperColors[i] = mix(c2, c3, i - m, COPPER_BANDS - 1 - m);
}

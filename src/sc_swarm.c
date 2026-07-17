#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "sound.h"

// scene 8: the sprite storm - 80 hardware sprites, the VDP limit,
// in shifting formations with fake scaling

#define NSPR   80
#define T_STAR (TILE_USER_INDEX + 0)
#define T_SPR  (TILE_USER_INDEX + 2)

static u16 sprTiles[4];

static void genStarTile(void)
{
    u32 tile[8];
    for (u16 r = 0; r < 8; r++) tile[r] = 0;
    tile[1] = 0x000E0000;
    tile[5] = 0x0000000E;
    VDP_loadTileData(tile, T_STAR, 1, DMA);
    for (u16 r = 0; r < 8; r++) tile[r] = 0;
    tile[3] = 0x00E00000;
    VDP_loadTileData(tile, T_STAR + 1, 1, DMA);
}

void swarm_init(void)
{
    demo_enterTiles();

    rnd_seed(0x5A5A);
    genStarTile();
    gen_triangleSprites(T_SPR, sprTiles);

    // starfield on B
    PAL_setColor(0, 0x0000);
    PAL_setColor(32 + 14, VCOL(4, 4, 5));
    for (u16 i = 0; i < 90; i++)
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL2, 0, 0, 0, T_STAR + (rnd() & 1)),
                         rnd() & 63, rnd() & 31);

    // sprite palette PAL3: neon triangle
    PAL_setColor(48 + 6, VCOL(0, 3, 6));
    PAL_setColor(48 + 15, VCOL(7, 7, 7));
    VDP_setBackgroundColor(0);

    snd_setMood(SND_HARD);
}

void swarm_update(u16 t)
{
    u16 phase = t / 260;
    if (phase > 2) phase = 2;

    for (u16 i = 0; i < NSPR; i++)
    {
        s16 x = 0, y = 0;
        u16 sz = 0;

        if (phase == 0)
        {
            // concentric rotating rings
            u16 ring = i >> 4;                       // 5 rings of 16
            u16 a = (t << 1) + (i << 4) + (ring << 5);
            s16 rad = 26 + ring * 20 + (SIN((t << 1) + (ring << 4)) >> 4);
            x = 160 + ((rad * COS(a)) >> 8);
            y = 108 + ((rad * SIN(a)) >> 8);
            sz = (ring > 2) ? 2 : ring;
        }
        else if (phase == 1)
        {
            // lissajous cloud
            x = 160 + ((110 * SIN(t + i * 7)) >> 8) + ((40 * SIN((t << 1) + i * 3)) >> 8);
            y = 108 + ((80 * SIN((t << 1) + i * 11 + 64)) >> 8);
            sz = ((i + (t >> 4)) & 3);
        }
        else
        {
            // 3D spiral burst flying into the screen
            u16 z = 280 - (((i << 3) + (t << 2)) & 255);
            u16 a = (i << 5) + t;
            x = 160 + (((90 * COS(a)) >> 8) * 160) / z;
            y = 108 + (((70 * SIN(a)) >> 8) * 160) / z;
            sz = 3 - ((z - 25) >> 6);
            if (sz > 3) sz = 3;
        }

        if (x < -32) x = -32;
        if (x > 336) x = 336;
        if (y < -32) y = -32;
        if (y > 240) y = 240;

        VDP_setSpriteFull(i, x, y, SPRITE_SIZE(sz + 1, sz + 1),
                          TILE_ATTR_FULL(PAL3, 0, 0, 0, sprTiles[sz]),
                          (i == NSPR - 1) ? 0 : (i + 1));
    }
    VDP_updateSprites(NSPR, DMA_QUEUE);

    // starfield blasts sideways, faster every phase
    VDP_setHorizontalScroll(BG_B, -(t << (2 + phase)));

    // triangle color storms through the spectrum
    if ((t & 1) == 0)
    {
        PAL_setColor(48 + 15, fx_hue((t << 1) & 255, 7));
        PAL_setColor(48 + 6, fx_hue(((t << 1) + 100) & 255, 5));
    }

    // formation switch flash
    if (t == 260 || t == 520)
        fx_fillPal(48, 2, VCOL(7, 7, 7));
}

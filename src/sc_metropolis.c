#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "sound.h"

// scene 7: vector megapolis - insane parallax scroller over procedural skylines

#define T_WIN   (TILE_USER_INDEX + 0)   // 4 window-pattern variants
#define T_SPR   (TILE_USER_INDEX + 8)   // triangle sprites (30 tiles)

static s16 lineA[224];
static s16 lineB[224];
static u16 sprTiles[4];
static s16 cutJump;

static void genWindowTiles(void)
{
    u32 tile[8];
    for (u16 v = 0; v < 4; v++)
    {
        for (u16 r = 0; r < 8; r++)
        {
            u32 w = 0;
            for (u16 c = 0; c < 8; c++)
            {
                u8 p = 1;
                // 2x2 windows on a 4px grid, randomly lit
                if ((r & 3) < 2 && (c & 3) < 2)
                    p = (rnd() & 3) ? 2 : 3;
                w = (w << 4) | p;
            }
            tile[r] = w;
        }
        VDP_loadTileData(tile, T_WIN + v, 1, DMA);
    }
}

static void buildSkyline(VDPPlane plane, u16 pal, u16 minH, u16 maxH, u16 gap)
{
    for (u16 x = 0; x < 64; x++)
    {
        if (gap && (rnd() % gap) == 0) continue;    // holes show the far layer
        u16 h = minH + rnd_range(maxH - minH);
        for (u16 y = 28 - h; y < 28; y++)
            VDP_setTileMapXY(plane,
                TILE_ATTR_FULL(pal, plane == BG_A, 0, 0, T_WIN + (rnd() & 3)), x, y);
    }
}

void metropolis_init(void)
{
    demo_enterTiles();
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

    rnd_seed(0xC17E);
    genWindowTiles();
    gen_triangleSprites(T_SPR, sprTiles);

    // PAL2 far layer (cold), PAL3 near layer (hot)
    PAL_setColor(0, VCOL(0, 0, 1));                 // deep night sky
    PAL_setColor(32 + 1, VCOL(1, 0, 2));
    PAL_setColor(32 + 2, VCOL(3, 2, 5));
    PAL_setColor(32 + 3, VCOL(5, 5, 7));
    PAL_setColor(48 + 1, VCOL(1, 0, 1));
    PAL_setColor(48 + 2, VCOL(7, 4, 1));
    PAL_setColor(48 + 3, VCOL(7, 7, 3));
    PAL_setColor(48 + 6, VCOL(4, 2, 6));
    PAL_setColor(48 + 15, VCOL(7, 7, 7));
    VDP_setBackgroundColor(0);

    buildSkyline(BG_B, PAL2, 6, 16, 0);
    buildSkyline(BG_A, PAL3, 10, 22, 3);

    cutJump = 0;
    // far layer sinks into hardware shadow - depth for free
    VDP_setHilightShadow(TRUE);
    snd_setMood(SND_HARD);
}

void metropolis_update(u16 t)
{
    // hard camera cuts every 150 frames
    if (t && (t % 150) == 0)
    {
        cutJump += 128 + (rnd() & 255);
        fx_fillPal(48, 4, VCOL(7, 7, 7));           // near layer flashes white
    }
    if ((t % 150) == 4)
    {
        PAL_setColor(48 + 1, VCOL(1, 0, 1));
        PAL_setColor(48 + 2, VCOL(7, 4, 1));
        PAL_setColor(48 + 3, VCOL(7, 7, 3));
    }

    s32 far = ((s32) t << 1) + cutJump;
    s32 near = ((s32) t * 6) + (cutJump << 1);

    // tear distortion: for a few frames after each cut a band of the
    // frame rips sideways
    u16 tearAge = t % 150;
    u16 tearY = 40 + ((t / 150) * 37) % 140;

    for (u16 y = 0; y < 224; y++)
    {
        lineB[y] = (s16) -(far >> ((y < 100) ? 1 : 0));
        lineA[y] = (s16) -((y >= 168) ? (near << 1) : near);
        if (tearAge < 10 && y >= tearY && y < tearY + 24)
        {
            lineA[y] += (10 - tearAge) * 9;
            lineB[y] -= (10 - tearAge) * 5;
        }
    }
    VDP_setHorizontalScrollLine(BG_B, 0, lineB, 224, DMA_QUEUE);
    VDP_setHorizontalScrollLine(BG_A, 0, lineA, 224, DMA_QUEUE);

    // vertical wobble
    VDP_setVerticalScroll(BG_A, SIN(t << 1) >> 6);

    // a triangle drone flies across, "scaling" through sprite sizes
    u16 ph = (t * 3) & 1023;
    if (ph < 512)
    {
        s16 sx = -32 + ph;
        s16 sy = 60 + (SIN(ph) >> 3);
        u16 sz = ph >> 7;                            // 0..3 - grows as it travels
        if (sz > 3) sz = 3;
        VDP_setSpriteFull(0, sx, sy, SPRITE_SIZE(sz + 1, sz + 1),
                          TILE_ATTR_FULL(PAL3, 1, 0, 0, sprTiles[sz]), 0);
    }
    else
        VDP_setSpriteFull(0, -32, 224, SPRITE_SIZE(1, 1),
                          TILE_ATTR_FULL(PAL3, 0, 0, 0, sprTiles[0]), 0);
    VDP_updateSprites(1, DMA_QUEUE);

    // neon windows cycle
    if ((t & 3) == 0)
    {
        PAL_setColor(32 + 2, fx_hue(t & 255, 4));
        PAL_setColor(48 + 2, fx_hue((t + 128) & 255, 7));
    }
}

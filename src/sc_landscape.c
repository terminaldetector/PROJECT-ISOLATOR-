#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "fxhint.h"
#include "sound.h"

// scene 6: pseudo mode7 - per-line hscroll turns a flat tile plane
// into a perspective floor rushing under the camera

#define HLINE 80                  // horizon scanline
#define T_GRID  (TILE_USER_INDEX + 0)
#define T_SOLID (TILE_USER_INDEX + 1)
#define T_MNT   (TILE_USER_INDEX + 2)   // 4 mountain silhouette variants
#define T_STAR  (TILE_USER_INDEX + 6)

static s16 lineB[224];
static s16 lineA[224];
static s16 persp[144];
static s32 camX;
static u16 vscroll;

static void genTiles(void)
{
    u32 tile[8];

    // grid tile: bright top/left edge on dark field
    for (u16 r = 0; r < 8; r++)
    {
        u32 w = 0;
        for (u16 c = 0; c < 8; c++)
        {
            u8 p = 1;
            if (r == 0 || c == 0) p = 2;
            w = (w << 4) | p;
        }
        tile[r] = w;
    }
    VDP_loadTileData(tile, T_GRID, 1, DMA);

    // solid black sky tile
    for (u16 r = 0; r < 8; r++) tile[r] = 0x11111111;
    VDP_loadTileData(tile, T_SOLID, 1, DMA);

    // mountain silhouettes: random jagged tops
    for (u16 v = 0; v < 4; v++)
    {
        u16 h1 = 2 + (rnd() & 3), h2 = 2 + (rnd() & 3);
        for (u16 r = 0; r < 8; r++)
        {
            u32 w = 0;
            for (u16 c = 0; c < 8; c++)
            {
                u16 h = (c < 4) ? h1 : h2;
                u8 p = (r >= 8 - h - ((c & 2) ? 1 : 0)) ? 3 : 1;
                w = (w << 4) | p;
            }
            tile[r] = w;
        }
        VDP_loadTileData(tile, T_MNT + v, 1, DMA);
    }

    // star tile: a couple of dots
    for (u16 r = 0; r < 8; r++) tile[r] = 0;
    tile[2] = 0x00040000;
    tile[6] = 0x40000002;
    VDP_loadTileData(tile, T_STAR, 1, DMA);
}

void landscape_init(void)
{
    demo_enterTiles();
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

    rnd_seed(0xBEEF);
    genTiles();

    // PAL2: floor, PAL3: sky
    PAL_setColor(32 + 1, VCOL(0, 0, 1));            // floor field
    PAL_setColor(32 + 2, VCOL(0, 5, 5));            // grid lines
    PAL_setColor(48 + 1, 0x0000);                   // sky black
    PAL_setColor(48 + 3, VCOL(2, 0, 4));            // mountains
    PAL_setColor(48 + 15, VCOL(7, 7, 7));           // stars (via tile color 4? unused)
    PAL_setColor(48 + 4, VCOL(6, 6, 7));
    VDP_setBackgroundColor(48 + 1);

    // floor covers the whole B plane
    VDP_fillTileMapRect(BG_B, TILE_ATTR_FULL(PAL2, 0, 0, 0, T_GRID), 0, 0, 64, 32);

    // sky on A: solid black rows with priority, mountains at the horizon row
    VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, 0, 0, T_SOLID), 0, 0, 64, 9);
    for (u16 x = 0; x < 64; x++)
    {
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL3, 1, 0, 0, T_MNT + (rnd() & 3)), x, 9);
        if ((rnd() & 7) == 0)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL3, 1, 0, 0, T_STAR), x, rnd() & 7);
    }

    // perspective table: scale grows with distance from horizon
    for (u16 i = 0; i < 144; i++)
        persp[i] = (i * 256) / 144;

    camX = 0;
    vscroll = 0;

    // copper sky: CRAM 49 is both the backdrop color and the solid sky tiles,
    // so the raster gradient paints the entire sky for free
    copper_enable(49);
    copper_gradient3(VCOL(0, 0, 2), VCOL(2, 0, 4), VCOL(6, 2, 1), 6);

    snd_setMood(SND_DRIVE);
}

void landscape_update(u16 t)
{
    // speed ramps up hard
    u16 spd = 4 + (t >> 6);
    if (spd > 14) spd = 14;
    camX += spd;

    // forward rush
    vscroll += (spd >> 1);
    VDP_setVerticalScroll(BG_B, vscroll);

    // curvature swings left/right
    s16 curve = SIN(t >> 1);

    for (u16 y = 0; y < 224; y++)
    {
        if (y < HLINE)
        {
            lineB[y] = 0;
            lineA[y] = (y > 56) ? (s16) -(camX >> 5) : 0;   // mountains parallax
        }
        else
        {
            u16 i = y - HLINE;
            s32 off = (camX * persp[i]) >> 7;
            off += ((s32) curve * ((144 - i) * (144 - i))) >> 13;
            lineB[y] = (s16) -off;
            lineA[y] = 0;
        }
    }

    VDP_setHorizontalScrollLine(BG_B, 0, lineB, 224, DMA_QUEUE);
    VDP_setHorizontalScrollLine(BG_A, 0, lineA, 224, DMA_QUEUE);

    // grid pulse + dawn slowly breaking across the copper sky
    if ((t & 3) == 0)
        PAL_setColor(32 + 2, fx_hue((t >> 1) & 255, 5 + ((t >> 5) & 1)));
    if ((t & 15) == 0)
    {
        u16 dawn = t >> 6;
        if (dawn > 6) dawn = 6;
        copper_gradient3(VCOL(0, 0, 2), VCOL(2, dawn >> 1, 4),
                         VCOL(1 + dawn, 1 + (dawn >> 1), 1), 6 + (dawn >> 1));
    }
}

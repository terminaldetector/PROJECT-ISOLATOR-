#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "sound.h"

// scene 10: digital rain of primitives - streams of generated glyphs

#define NCOL     40
#define ROWS     28
#define T_GLYPH  (TILE_USER_INDEX + 0)   // 8 glyph variants
#define T_GRID   (TILE_USER_INDEX + 8)

static s16 head[NCOL];
static u8  speed[NCOL];
static u8  timer[NCOL];

static void genGlyphTiles(void)
{
    u32 tile[8];
    for (u16 v = 0; v < 8; v++)
    {
        // tiny primitives: triangles, bars, crosses - assembled from random strokes
        u8 px[8][8];
        memset(px, 0, 64);
        u16 kind = v & 3;
        for (u16 r = 0; r < 8; r++)
            for (u16 c = 0; c < 8; c++)
            {
                u16 on = 0;
                switch (kind)
                {
                    case 0: on = (r >= 6 - (c < 4 ? c : 7 - c) && r < 7); break;  // triangle
                    case 1: on = (r == 1 || r == 6 || c == 1 || c == 6) &&
                                 (r > 0 && r < 7 && c > 0 && c < 7); break;        // box
                    case 2: on = (r == c || r == 7 - c); break;                    // cross
                    case 3: on = ((r ^ c) & 2) == 0 && (rnd() & 3) == 0; break;    // noise
                }
                if (on && (rnd() & 7)) px[r][c] = 2;
            }
        for (u16 r = 0; r < 8; r++)
        {
            u32 w = 0;
            for (u16 c = 0; c < 8; c++) w = (w << 4) | px[r][c];
            tile[r] = w;
        }
        VDP_loadTileData(tile, T_GLYPH + v, 1, DMA);
    }

    // faint background grid
    for (u16 r = 0; r < 8; r++)
        tile[r] = (r == 0) ? 0x11111111 : 0x10000000;
    VDP_loadTileData(tile, T_GRID, 1, DMA);
}

void rain_init(void)
{
    demo_enterTiles();

    rnd_seed(0xD1CE);
    genGlyphTiles();

    // PAL2 bright stream, PAL3 dim tail
    PAL_setColor(0, 0x0000);
    PAL_setColor(32 + 2, VCOL(3, 7, 4));
    PAL_setColor(48 + 2, VCOL(0, 3, 1));
    PAL_setColor(48 + 1, VCOL(0, 1, 1));
    VDP_setBackgroundColor(0);

    VDP_fillTileMapRect(BG_B, TILE_ATTR_FULL(PAL3, 0, 0, 0, T_GRID), 0, 0, 64, 32);

    for (u16 c = 0; c < NCOL; c++)
    {
        head[c] = -(s16) rnd_range(40);
        speed[c] = 1 + (rnd() & 2);
        timer[c] = speed[c];
    }

    snd_setMood(SND_CALM);
}

void rain_update(u16 t)
{
    for (u16 c = 0; c < NCOL; c++)
    {
        if (--timer[c]) continue;
        timer[c] = speed[c];

        head[c]++;
        s16 h = head[c];
        if (h - 10 > ROWS)
        {
            head[c] = -(s16) rnd_range(30);
            speed[c] = 1 + (rnd() & 2);
            continue;
        }

        // bright head
        if (h >= 0 && h < ROWS)
            VDP_setTileMapXY(BG_A,
                TILE_ATTR_FULL(PAL2, 0, 0, 0, T_GLYPH + (rnd() & 7)), c, h);
        // body dims
        if (h - 3 >= 0 && h - 3 < ROWS)
            VDP_setTileMapXY(BG_A,
                TILE_ATTR_FULL(PAL3, 0, 0, 0, T_GLYPH + (rnd() & 7)), c, h - 3);
        // tail evaporates
        if (h - 9 >= 0 && h - 9 < ROWS)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, 0), c, h - 9);
    }

    // background grid drifts down slowly
    VDP_setVerticalScroll(BG_B, -(t >> 3));

    // the green digital glow breathes
    if ((t & 7) == 0)
    {
        u16 g = 6 + ((SIN(t << 1) >> 7));
        if (g > 7) g = 7;
        PAL_setColor(32 + 2, VCOL(2, g, 3));
    }
}

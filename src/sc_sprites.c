#include <genesis.h>
#include "scenes.h"
#include "util.h"

// procedural sprite factory: triangle glyphs at 8/16/24/32 px,
// rendered into a pixel buffer at runtime and packed into 4bpp tiles

#define MAXPX 32

static u8 pix[MAXPX * MAXPX];

static void ppset(u16 n, s16 x, s16 y, u8 col)
{
    if (x >= 0 && y >= 0 && x < (s16) n && y < (s16) n)
        pix[y * n + x] = col;
}

static void pline(u16 n, s16 x1, s16 y1, s16 x2, s16 y2, u8 col)
{
    s16 dx = x2 - x1, dy = y2 - y1;
    s16 adx = dx < 0 ? -dx : dx;
    s16 ady = dy < 0 ? -dy : dy;
    s16 steps = (adx > ady) ? adx : ady;
    if (steps == 0)
    {
        ppset(n, x1, y1, col);
        return;
    }
    for (s16 i = 0; i <= steps; i++)
        ppset(n, x1 + (dx * i) / steps, y1 + (dy * i) / steps, col);
}

// pack an NxN pixel buffer into MD sprite tiles (column-major tile order)
static void packAndLoad(u16 n, u16 vramIndex)
{
    static u32 tiles[16 * 8];   // up to 4x4 tiles
    u16 c = n >> 3;
    u16 ti = 0;

    for (u16 cx = 0; cx < c; cx++)
        for (u16 cy = 0; cy < c; cy++)
        {
            for (u16 row = 0; row < 8; row++)
            {
                u32 w = 0;
                for (u16 col = 0; col < 8; col++)
                {
                    u8 p = pix[(cy * 8 + row) * n + cx * 8 + col] & 15;
                    w = (w << 4) | p;
                }
                tiles[ti * 8 + row] = w;
            }
            ti++;
        }

    VDP_loadTileData(tiles, vramIndex, c * c, DMA);
}

void gen_triangleSprites(u16 baseTile, u16 *tileIndex)
{
    static const u16 sizes[4] = { 8, 16, 24, 32 };
    u16 vi = baseTile;

    for (u16 s = 0; s < 4; s++)
    {
        u16 n = sizes[s];
        memset(pix, 0, n * n);

        // triangle outline, bright rim + dim inner echo
        s16 top = 0, bot = n - 1, mid = n >> 1;
        pline(n, mid, top, 0, bot, 15);
        pline(n, mid, top, n - 1, bot, 15);
        pline(n, 0, bot, n - 1, bot, 15);
        if (n > 8)
        {
            pline(n, mid, top + 3, 3, bot - 2, 6);
            pline(n, mid, top + 3, n - 4, bot - 2, 6);
            pline(n, 3, bot - 2, n - 4, bot - 2, 6);
        }
        // the eye dot
        ppset(n, mid, (n * 2) / 3, 15);

        packAndLoad(n, vi);
        tileIndex[s] = vi;
        vi += (n >> 3) * (n >> 3);
    }
}

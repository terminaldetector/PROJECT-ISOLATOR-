#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "sound.h"

// scene 11: the world breaks - glitch, shatter into triangles,
// whiteout, and the reveal

#define T_JUNK  (TILE_USER_INDEX + 0)   // 8 garbage tiles
#define T_SPR   (TILE_USER_INDEX + 8)

#define NPART 80

static s16 lineA[224];
static u16 sprTiles[4];
static s16 px8[NPART], py8[NPART], vx8[NPART], vy8[NPART];

static void genJunkTiles(void)
{
    u32 tile[8];
    for (u16 v = 0; v < 8; v++)
    {
        for (u16 r = 0; r < 8; r++)
        {
            u32 w = 0;
            for (u16 c = 0; c < 8; c++)
                w = (w << 4) | ((rnd() & 7) ? 0 : (rnd() & 15));
            tile[r] = w;
        }
        VDP_loadTileData(tile, T_JUNK + v, 1, DMA);
    }
}

static void drawCentered(const char *s, u16 y)
{
    u16 len = strlen(s);
    VDP_drawText(s, (40 - len) >> 1, y);
}

void finale_init(void)
{
    demo_enterTiles();
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

    genJunkTiles();
    gen_triangleSprites(T_SPR, sprTiles);

    // the dying world: junk everywhere
    PAL_setColor(0, 0x0000);
    PAL_setColor(2, VCOL(2, 6, 4));
    PAL_setColor(5, VCOL(6, 2, 5));
    PAL_setColor(10, VCOL(3, 3, 7));
    PAL_setColor(15, VCOL(7, 7, 7));
    PAL_setColor(48 + 6, VCOL(0, 3, 6));
    PAL_setColor(48 + 15, VCOL(7, 7, 7));
    VDP_setBackgroundColor(0);

    for (u16 i = 0; i < 300; i++)
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, T_JUNK + (rnd() & 7)),
                         rnd() & 63, rnd() & 31);

    // shatter particles: burst from screen center, 13.3 fixed point
    for (u16 i = 0; i < NPART; i++)
    {
        px8[i] = 160 << 3;
        py8[i] = 104 << 3;
        u16 a = rnd() & 255;
        u16 v = 8 + (rnd() & 31);
        vx8[i] = (v * COS(a)) >> 8;
        vy8[i] = ((v * SIN(a)) >> 8) - 8;
    }

    snd_setMood(SND_DRIVE);
}

void finale_update(u16 t)
{
    if (t < 180)
    {
        // PHASE A: glitch avalanche
        u16 n = 2 + (t >> 4);
        for (u16 y = 0; y < 224; y++) lineA[y] = 0;
        for (u16 i = 0; i < n; i++)
        {
            u16 y = rnd() % 220;
            s16 off = (s16)(rnd() & 63) - 32;
            for (u16 k = 0; k < 4 && y + k < 224; k++) lineA[y + k] = off;
        }
        VDP_setHorizontalScrollLine(BG_A, 0, lineA, 224, DMA_QUEUE);

        // more corruption every frame
        for (u16 i = 0; i < n; i++)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, T_JUNK + (rnd() & 7)),
                             rnd() & 63, rnd() & 31);

        // strobing palette panic
        if ((t & 7) < 2)
            PAL_setColor(0, fx_hue(rnd() & 255, 2));
        else
            PAL_setColor(0, 0x0000);

        PSG_setNoise(1, rnd() & 3);
        PSG_setEnvelope(3, 8 + (rnd() & 5));
    }
    else if (t == 180)
    {
        // PHASE B: shatter
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);
        for (u16 y = 0; y < 224; y++) lineA[y] = 0;
        VDP_setHorizontalScrollLine(BG_A, 0, lineA, 224, DMA_QUEUE);
        PAL_setColor(0, 0x0000);
        PSG_setEnvelope(3, 15);
        snd_boom();
    }
    else if (t < 430)
    {
        // particles fly and fall
        for (u16 i = 0; i < NPART; i++)
        {
            px8[i] += vx8[i];
            py8[i] += vy8[i];
            vy8[i] += 1;                              // gravity
            s16 x = px8[i] >> 3, y = py8[i] >> 3;
            if (x < -32) x = -32;
            if (x > 336) x = 336;
            if (y > 240) y = 240;
            u16 sz = (i & 3);
            VDP_setSpriteFull(i, x, y, SPRITE_SIZE(sz + 1, sz + 1),
                              TILE_ATTR_FULL(PAL3, 0, 0, 0, sprTiles[sz]),
                              (i == NPART - 1) ? 0 : (i + 1));
        }
        VDP_updateSprites(NPART, DMA_QUEUE);

        // PHASE C: whiteout builds from t=320
        if (t >= 320 && (t & 7) == 0)
        {
            u16 w = (t - 320) >> 3;                   // 0..13
            u16 v = w > 7 ? 7 : w;
            PAL_setColor(0, VCOL(v, v, v));
            fx_fillPal(48 + 4, 2, VCOL(7, 7, v));
        }
    }
    else if (t == 430)
    {
        // PHASE D: white silence
        VDP_clearSprites();
        VDP_updateSprites(1, DMA);
        fx_allWhite();
        PAL_setColor(15, VCOL(1, 1, 3));              // ink on white
        VDP_setTextPalette(PAL0);
        VDP_setBackgroundColor(0);
        snd_setMood(SND_OFF);
    }
    else
    {
        // the reveal, line by line
        if (t == 460) drawCentered("ROM GENERATED", 8);
        if (t == 520) drawCentered("100%", 10);
        if (t == 610)
        {
            VDP_clearPlane(BG_A, TRUE);
            drawCentered("SEGA MEGA DRIVE", 5);
            drawCentered("AI TECH DEMO", 7);
        }
        if (t == 640) drawCentered("NO PRECOMPUTED LEVELS", 12);
        if (t == 670) drawCentered("ONLY GENERATED SCENES", 14);
        if (t == 730) drawCentered("SYNTAXIS : FRACTAL GENESIS", 19);
        if (t > 760)
            drawCentered(((t >> 5) & 1) ? "PRESS START TO REGENERATE" : "                         ", 24);

        // fade to black right before the loop restarts
        if (t > 930 && (t & 3) == 0)
        {
            u16 v = 7 - ((t - 930) / 7);
            if (v > 7) v = 0;
            fx_fillPal(0, 1, VCOL(v, v, v));
            u16 ink = v >> 2;
            PAL_setColor(15, VCOL(ink, ink, ink + (v > 2 ? 1 : 0)));
        }
    }
}

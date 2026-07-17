#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "draw3d.h"
#include "sound.h"
#include "seq.h"

static u16 built;

// scene 3: the AI builds a wireframe city around the eye, camera orbits

#define MAX_B  10
#define GROUND -40

typedef struct
{
    s16 x, z;       // center
    s16 w;          // half width
    s16 h;          // height
} Building;

static Building bld[MAX_B];

static void drawBuilding(const Building *b, u8 col)
{
    V3 v[8];
    for (u16 i = 0; i < 8; i++)
    {
        v[i].x = b->x + ((i & 1) ? b->w : -b->w);
        v[i].z = b->z + ((i & 2) ? b->w : -b->w);
        v[i].y = (i & 4) ? (GROUND + b->h) : GROUND;
    }
    // vertical edges
    d3_line(&v[0], &v[4], col);
    d3_line(&v[1], &v[5], col);
    d3_line(&v[2], &v[6], col);
    d3_line(&v[3], &v[7], col);
    // roof: dithered solid slab when it faces the camera
    s16 rx[4], ry[4];
    u16 ok = d3_project(&v[4], &rx[0], &ry[0]) && d3_project(&v[5], &rx[1], &ry[1])
          && d3_project(&v[7], &rx[2], &ry[2]) && d3_project(&v[6], &rx[3], &ry[3]);
    if (ok && !d3_backface(rx[0], ry[0], rx[1], ry[1], rx[2], ry[2]))
    {
        bmp_fillTri(rx[0], ry[0], rx[1], ry[1], rx[2], ry[2], col, 1);
        bmp_fillTri(rx[0], ry[0], rx[2], ry[2], rx[3], ry[3], col, 1);
    }
    else if (ok)
    {
        bmp_lineSafe(rx[0], ry[0], rx[1], ry[1], col);
        bmp_lineSafe(rx[1], ry[1], rx[2], ry[2], col);
        bmp_lineSafe(rx[2], ry[2], rx[3], ry[3], col);
        bmp_lineSafe(rx[3], ry[3], rx[0], ry[0], col);
    }
}

void city_init(void)
{
    demo_enterBMP();

    PAL_setColor(16 + 0, 0x0000);
    PAL_setColor(16 + 1, VCOL(0, 1, 2));
    PAL_setColor(16 + 2, VCOL(0, 4, 5));
    PAL_setColor(16 + 3, VCOL(2, 6, 7));
    PAL_setColor(16 + 4, VCOL(6, 3, 7));
    PAL_setColor(16 + 15, VCOL(7, 7, 7));
    VDP_setBackgroundColor(16);

    // the AI "decides" the city layout
    for (u16 i = 0; i < MAX_B; i++)
    {
        bld[i].x = -120 + (i % 5) * 60 + (rnd() & 15);
        bld[i].z = -60 + (i / 5) * 90 + (rnd() & 31);
        bld[i].w = 14 + (rnd() & 7);
        bld[i].h = 30 + rnd_range(70);
    }

    built = 0;
    snd_setMood(SND_DRIVE);
}

void city_update(u16 t)
{
    BMP_waitWhileFlipRequestPending();
    BMP_clear();

    // slow orbit with a light vertical bob
    u16 ay = t;
    u16 ax = 12 + (SIN(t << 1) >> 6);
    d3_setCamera(ax, ay, 260 - (t >> 3 > 40 ? 40 : t >> 3));

    // ground grid
    V3 a, b;
    for (s16 g = -150; g <= 150; g += 50)
    {
        a.x = g; a.y = GROUND; a.z = -150;
        b.x = g; b.y = GROUND; b.z = 150;
        d3_line(&a, &b, 1);
        a.x = -150; a.z = g;
        b.x = 150;  b.z = g;
        d3_line(&a, &b, 1);
    }

    // the AI builds on the kick: a new tower rises with every other hit
    if (seq_isKick() && (seq_step() == 0 || seq_step() == 8) && built < MAX_B)
        built++;
    if (built == 0 && t > 90) built = 1;
    for (u16 i = 0; i < built; i++)
    {
        u8 col = 2;
        if (i == built - 1 && built < MAX_B)
            col = ((t & 4) ? 15 : 3);      // construction flicker
        drawBuilding(&bld[i], col);
    }

    // the scan beam answers the snare
    if (seq_isSnare())
    {
        u16 sweep = (t << 3) & 255;
        bmp_lineSafe(sweep, 0, sweep, 159, 15);
    }

    // data noise
    for (u16 i = 0; i < 3; i++)
        BMP_setPixel(rnd() & 255, rnd_range(160), BCOL(2));

    BMP_flip(1);

    // neon palette drift
    if ((t & 7) == 0)
        PAL_setColor(16 + 4, fx_hue((t >> 2) & 255, 7));
}

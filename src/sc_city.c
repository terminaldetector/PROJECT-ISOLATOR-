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

// box faces (vertex indices) and their dithered shading: front lit windows,
// sides shadowed windows, back dark, roof bright solid
static const u8 faceIdx[5][4] =
{
    { 0, 1, 5, 4 },     // -z front
    { 1, 3, 7, 5 },     // +x right
    { 3, 2, 6, 7 },     // +z back
    { 2, 0, 4, 6 },     // -x left
    { 4, 5, 7, 6 },     // +y roof
};
static const u8 faceShade[5][2] =
{
    { 7, 8 },           // front: bright cyan / dark window
    { 5, 8 },           // right: dim / window
    { 4, 8 },           // back: darkest
    { 5, 8 },           // left: dim / window
    { 7, 6 },           // roof: solid bright
};

static void drawBuilding(const Building *b, u8 lit)
{
    V3 v[8];
    s16 px[8], py[8];
    for (u16 i = 0; i < 8; i++)
    {
        v[i].x = b->x + ((i & 1) ? b->w : -b->w);
        v[i].z = b->z + ((i & 2) ? b->w : -b->w);
        v[i].y = (i & 4) ? (GROUND + b->h) : GROUND;
        if (!d3_project(&v[i], &px[i], &py[i])) return;
    }

    // sort the 5 faces far -> near by centroid depth
    s16 fd[5];
    u8 ord[5];
    for (u16 f = 0; f < 5; f++)
    {
        const u8 *q = faceIdx[f];
        V3 c;
        c.x = (v[q[0]].x + v[q[1]].x + v[q[2]].x + v[q[3]].x) >> 2;
        c.y = (v[q[0]].y + v[q[1]].y + v[q[2]].y + v[q[3]].y) >> 2;
        c.z = (v[q[0]].z + v[q[1]].z + v[q[2]].z + v[q[3]].z) >> 2;
        fd[f] = d3_depth(&c);
        ord[f] = f;
    }
    for (u16 i = 1; i < 5; i++)
    {
        u8 k = ord[i];
        s16 d = fd[k];
        s16 j = i;
        while (j > 0 && fd[ord[j - 1]] < d) { ord[j] = ord[j - 1]; j--; }
        ord[j] = k;
    }

    // draw only the 3 nearest faces - exactly the visible ones for a box
    for (u16 s = 2; s < 5; s++)
    {
        const u8 *q = faceIdx[ord[s]];
        u8 cA = faceShade[ord[s]][0], cB = faceShade[ord[s]][1];
        bmp_fillTri(px[q[0]], py[q[0]], px[q[1]], py[q[1]], px[q[2]], py[q[2]], cA, cB);
        bmp_fillTri(px[q[0]], py[q[0]], px[q[2]], py[q[2]], px[q[3]], py[q[3]], cA, cB);
    }

    // neon edge accents over the solid mass
    bmp_lineSafe(px[4], py[4], px[5], py[5], lit);
    bmp_lineSafe(px[5], py[5], px[7], py[7], lit);
    bmp_lineSafe(px[7], py[7], px[6], py[6], lit);
    bmp_lineSafe(px[6], py[6], px[4], py[4], lit);
    bmp_lineSafe(px[0], py[0], px[4], py[4], lit);
    bmp_lineSafe(px[1], py[1], px[5], py[5], lit);
    bmp_lineSafe(px[3], py[3], px[7], py[7], lit);
}

void city_init(void)
{
    demo_enterBMP();

    PAL_setColor(16 + 0, 0x0000);
    PAL_setColor(16 + 1, VCOL(0, 1, 2));     // ground grid
    PAL_setColor(16 + 2, VCOL(0, 4, 5));     // neon edge base
    PAL_setColor(16 + 3, VCOL(2, 6, 7));
    PAL_setColor(16 + 4, VCOL(6, 3, 7));     // magenta neon (newest tower)
    PAL_setColor(16 + 5, VCOL(0, 2, 3));     // dim wall
    PAL_setColor(16 + 6, VCOL(1, 4, 5));     // mid wall
    PAL_setColor(16 + 7, VCOL(2, 6, 7));     // lit wall
    PAL_setColor(16 + 8, VCOL(0, 1, 2));     // window (dark, dithers into walls)
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

    // painter sort the visible towers back -> front by center depth
    s16 bd[MAX_B];
    u8 border[MAX_B];
    for (u16 i = 0; i < built; i++)
    {
        V3 c = { bld[i].x, GROUND + (bld[i].h >> 1), bld[i].z };
        bd[i] = d3_depth(&c);
        border[i] = i;
    }
    for (u16 i = 1; i < built; i++)
    {
        u8 k = border[i];
        s16 d = bd[k];
        s16 j = i;
        while (j > 0 && bd[border[j - 1]] < d) { border[j] = border[j - 1]; j--; }
        border[j] = k;
    }
    for (u16 s = 0; s < built; s++)
    {
        u16 i = border[s];
        u8 lit = 2;
        if (i == built - 1 && built < MAX_B)
            lit = ((t & 4) ? 15 : 4);      // the newest tower flickers neon
        drawBuilding(&bld[i], lit);
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

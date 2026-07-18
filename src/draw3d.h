#ifndef DRAW3D_H
#define DRAW3D_H

#include <genesis.h>

typedef struct
{
    s16 x, y, z;
} V3;

void d3_setCamera(u16 ax, u16 ay, s16 dist);
u16  d3_project(const V3 *v, s16 *sx, s16 *sy);
void d3_line(const V3 *a, const V3 *b, u8 col);

// line clipped to the BMP area
void bmp_lineSafe(s16 x1, s16 y1, s16 x2, s16 y2, u8 col);

// horizontal span with an explicit 2-pixel byte pattern
void bmp_hspan(s16 x1, s16 x2, s16 y, u8 pattern);

// filled triangle with checker dithering between palette colors c1/c2
// (pass c1 == c2 for a solid fill)
void bmp_fillTri(s16 x1, s16 y1, s16 x2, s16 y2, s16 x3, s16 y3, u8 c1, u8 c2);

// filled disc / flattened ellipse, checker-dithered between two palette colors
void bmp_disc(s16 cx, s16 cy, s16 r, u8 cA, u8 cB);
void bmp_ellipse(s16 cx, s16 cy, s16 rx, s16 ry, u8 cA, u8 cB);

// TRUE when the projected triangle winds away from the camera
u16 d3_backface(s16 x1, s16 y1, s16 x2, s16 y2, s16 x3, s16 y3);

// camera-space depth of a point (larger = farther) for painter sorting
s16 d3_depth(const V3 *v);

#endif

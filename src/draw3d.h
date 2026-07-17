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

#endif

#include "draw3d.h"
#include "util.h"

static s16 cs_ax = 256, sn_ax = 0;
static s16 cs_ay = 256, sn_ay = 0;
static s16 camDist = 200;

void d3_setCamera(u16 ax, u16 ay, s16 dist)
{
    cs_ax = COS(ax);
    sn_ax = SIN(ax);
    cs_ay = COS(ay);
    sn_ay = SIN(ay);
    camDist = dist;
}

u16 d3_project(const V3 *v, s16 *sx, s16 *sy)
{
    // rotate around Y, then around X, then translate and project
    s32 x = ((s32) v->x * cs_ay + (s32) v->z * sn_ay) >> 8;
    s32 z = ((s32) v->z * cs_ay - (s32) v->x * sn_ay) >> 8;
    s32 y = ((s32) v->y * cs_ax - z * sn_ax) >> 8;
    z = (((s32) v->y * sn_ax + z * cs_ax) >> 8) + camDist;
    if (z < 24) return FALSE;

    s32 px = 128 + ((x * 160) / z);
    s32 py = 80 - ((y * 160) / z);
    if ((px < -320) || (px > 640) || (py < -320) || (py > 480)) return FALSE;
    *sx = (s16) px;
    *sy = (s16) py;
    return TRUE;
}

void bmp_lineSafe(s16 x1, s16 y1, s16 x2, s16 y2, u8 col)
{
    Line l;
    l.pt1.x = x1;
    l.pt1.y = y1;
    l.pt2.x = x2;
    l.pt2.y = y2;
    l.col = BCOL(col);
    if (BMP_clipLine(&l)) BMP_drawLine(&l);
}

void d3_line(const V3 *a, const V3 *b, u8 col)
{
    s16 x1, y1, x2, y2;
    if (d3_project(a, &x1, &y1) && d3_project(b, &x2, &y2))
        bmp_lineSafe(x1, y1, x2, y2, col);
}

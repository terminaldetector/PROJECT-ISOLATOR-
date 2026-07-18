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

void bmp_hspan(s16 x1, s16 x2, s16 y, u8 pattern)
{
    if (y < 0 || y > 159) return;
    if (x1 > x2) { s16 t = x1; x1 = x2; x2 = t; }
    if (x2 < 0 || x1 > 255) return;
    if (x1 < 0) x1 = 0;
    if (x2 > 255) x2 = 255;
    Line l;
    l.pt1.x = x1; l.pt1.y = y;
    l.pt2.x = x2; l.pt2.y = y;
    l.col = pattern;
    BMP_drawLine(&l);
}

// scanline-filled triangle, checker-dithered between two palette colors
void bmp_fillTri(s16 x1, s16 y1, s16 x2, s16 y2, s16 x3, s16 y3, u8 c1, u8 c2)
{
    s16 tx, ty;
    // sort vertices by y
    if (y1 > y2) { ty = y1; y1 = y2; y2 = ty; tx = x1; x1 = x2; x2 = tx; }
    if (y2 > y3) { ty = y2; y2 = y3; y3 = ty; tx = x2; x2 = x3; x3 = tx; }
    if (y1 > y2) { ty = y1; y1 = y2; y2 = ty; tx = x1; x1 = x2; x2 = tx; }
    if (y3 < 0 || y1 > 159 || y1 == y3) return;

    u8 even = BCOL2(c1, c2);
    u8 odd  = BCOL2(c2, c1);

    // long edge y1->y3, split edges y1->y2, y2->y3 (8.8 fixed point walk)
    s32 xl = (s32) x1 << 8;
    s32 dl = (((s32)(x3 - x1)) << 8) / (y3 - y1);
    s32 xs, ds;

    if (y2 > y1)
    {
        xs = (s32) x1 << 8;
        ds = (((s32)(x2 - x1)) << 8) / (y2 - y1);
        for (s16 y = y1; y < y2; y++)
        {
            bmp_hspan(xl >> 8, xs >> 8, y, (y & 1) ? odd : even);
            xl += dl;
            xs += ds;
        }
    }
    if (y3 > y2)
    {
        xs = (s32) x2 << 8;
        ds = (((s32)(x3 - x2)) << 8) / (y3 - y2);
        for (s16 y = y2; y <= y3; y++)
        {
            bmp_hspan(xl >> 8, xs >> 8, y, (y & 1) ? odd : even);
            xl += dl;
            xs += ds;
        }
    }
    else
        bmp_hspan(xl >> 8, x3, y3, (y3 & 1) ? odd : even);
}

void bmp_ellipse(s16 cx, s16 cy, s16 rx, s16 ry, u8 cA, u8 cB)
{
    if (rx < 1 || ry < 1) { BMP_setPixel(cx, cy, BCOL2(cA, cB)); return; }
    s32 rx2 = (s32) rx * rx;
    for (s16 yy = -ry; yy <= ry; yy++)
    {
        // half-width at this row: rx * sqrt(1 - yy^2/ry^2)
        s32 t = rx2 - (rx2 * yy * yy) / ((s32) ry * ry);
        s16 w = (t > 0) ? (s16) isqrt32((u32) t) : 0;
        bmp_hspan(cx - w, cx + w, cy + yy, (yy & 1) ? BCOL2(cA, cB) : BCOL2(cB, cA));
    }
}

void bmp_disc(s16 cx, s16 cy, s16 r, u8 cA, u8 cB)
{
    if (r < 1) { BMP_setPixel(cx, cy, BCOL2(cA, cB)); return; }
    s32 r2 = (s32) r * r;
    for (s16 yy = -r; yy <= r; yy++)
    {
        s32 t = r2 - (s32) yy * yy;
        s16 w = (t > 0) ? (s16) isqrt32((u32) t) : 0;
        bmp_hspan(cx - w, cx + w, cy + yy, (yy & 1) ? BCOL2(cA, cB) : BCOL2(cB, cA));
    }
}

void bmp_vgradRamp(s16 y0, s16 y1, const u8 *idx, u16 n)
{
    if (y1 <= y0 || n == 0) return;
    if (y0 < 0) y0 = 0;
    if (y1 > 160) y1 = 160;
    u16 span = y1 - y0;
    for (s16 y = y0; y < y1; y++)
    {
        u16 f = ((u16)(y - y0) * (n - 1) * 2) / span;   // 0 .. 2(n-1)
        u16 lo = f >> 1;
        u8 a = idx[lo];
        u8 b = ((f & 1) && (lo + 1 < n)) ? idx[lo + 1] : a;
        bmp_hspan(0, 255, y, (y & 1) ? BCOL2(a, b) : BCOL2(b, a));
    }
}

u16 d3_backface(s16 x1, s16 y1, s16 x2, s16 y2, s16 x3, s16 y3)
{
    s32 cross = (s32)(x2 - x1) * (y3 - y1) - (s32)(y2 - y1) * (x3 - x1);
    return cross >= 0;
}

s16 d3_depth(const V3 *v)
{
    s32 zc = ((s32) v->z * cs_ay - (s32) v->x * sn_ay) >> 8;
    s32 zf = (((s32) v->y * sn_ax + zc * cs_ax) >> 8) + camDist;
    return (s16) zf;
}

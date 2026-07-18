#ifndef UTIL_H
#define UTIL_H

#include <genesis.h>

// runtime-generated sine table: 256 steps per revolution, amplitude 256
extern s16 g_sin[256];
void util_init(void);

#define SIN(a)   g_sin[(a) & 255]
#define COS(a)   g_sin[((a) + 64) & 255]

void rnd_seed(u16 s);
u16  rnd(void);
u16  rnd_range(u16 n);

// fast bit-by-bit integer square root (floor), O(1) fixed iterations -
// no division, no linear search. Used everywhere a circle/ellipse fill
// needs its half-width per scanline; this is the single biggest fps win
// for the disc-heavy scenes (mushroom cap, tunnel sphere).
u16 isqrt32(u32 n);

// r,g,b in 0..7 -> VDP color word
#define VCOL(r, g, b)  (((r) << 1) | ((g) << 5) | ((b) << 9))

// 4bpp byte pattern (2 pixels) for the BMP engine
#define BCOL(c)  (((c) << 4) | (c))
// two-color pattern: free vertical-stripe dithering; alternate per row
// for a checkerboard - doubles the apparent palette depth
#define BCOL2(a, b)  (((a) << 4) | (b))

#endif

#ifndef FXHINT_H
#define FXHINT_H

#include <genesis.h>

// copper-style raster gradient: the background color is rewritten every
// 8 scanlines from a 28-entry band table - a full-screen sky gradient
// that costs zero palette entries

#define COPPER_BANDS 28

extern u16 copperColors[COPPER_BANDS];

// palIndex: CRAM entry used as the screen background color
// (0 in tile scenes, 16 in BMP scenes)
void copper_enable(u16 palIndex);
void copper_disable(void);

// fill the band table with a vertical gradient c1 (top) -> c2 (bottom)
void copper_gradient(u16 c1, u16 c2);
// three-stop gradient: top -> mid (at band m) -> bottom
void copper_gradient3(u16 c1, u16 c2, u16 c3, u16 m);

#endif

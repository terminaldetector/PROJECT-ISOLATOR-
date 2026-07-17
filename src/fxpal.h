#ifndef FXPAL_H
#define FXPAL_H

#include <genesis.h>

void fx_gradient(u16 index, u16 count, u16 c1, u16 c2);
void fx_fillPal(u16 index, u16 count, u16 color);
void fx_allBlack(void);
void fx_allWhite(void);

// rainbow color from a 0..255 hue, brightness 0..7
u16 fx_hue(u8 h, u16 bright);

#endif

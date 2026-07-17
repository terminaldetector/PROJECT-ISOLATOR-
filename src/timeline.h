#ifndef TIMELINE_H
#define TIMELINE_H

#include <genesis.h>

typedef struct
{
    void (*init)(void);
    void (*update)(u16 t);
    u16 duration;           // in frames
} Scene;

extern u32 g_frame;

// switch the VDP between the two big modes the demo uses
void demo_enterTiles(void);
void demo_enterBMP(void);

void timeline_run(void);

#endif

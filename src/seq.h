#ifndef SEQ_H
#define SEQ_H

#include <genesis.h>

// track sections
#define SEC_OFF     0
#define SEC_CALM    1   // bell arps, pads - the world forming
#define SEC_DRIVE   2   // full speed: bass 16ths, stabs, lead
#define SEC_HARD    3   // doubled kick, unison leads - the hardcore
#define SEC_AGONY   4   // half-time elegy, crying lead - the flower
#define SEC_MYTHIC  5   // solemn hardcore hymn - tree / aleph / spark

void seq_init(void);
void seq_setSection(u8 sec);
void seq_tick(void);
void seq_boom(void);

// sync hooks for the visuals
u16 seq_step(void);         // 16th inside the bar (0..15)
u16 seq_bar(void);          // bar counter
u16 seq_isKick(void);       // TRUE on the exact frame a kick fires
u16 seq_isSnare(void);
u16 seq_isDownbeat(void);   // TRUE on the first frame of a bar

#endif

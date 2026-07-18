#ifndef FM_H
#define FM_H

#include <genesis.h>

// direct 68k driver for the YM2612 - the Genesis "soundfont" itself.
// operators are stored in register order: OP1, OP3, OP2, OP4.

typedef struct
{
    u8 alg, fb;
    u8 dtmul[4];    // DT(3)<<4 | MUL(4)
    u8 tl[4];       // total level, 0 = loudest (carriers get vel added)
    u8 rsar[4];     // RS(2)<<6 | AR(5)
    u8 d1r[4];      // AM(1)<<7 | D1R(5)
    u8 d2r[4];      // D2R(5)
    u8 slrr[4];     // SL(4)<<4 | RR(4)
} FMPatch;

// the instrument bank, defined in fm.c
extern const FMPatch fmBass;
extern const FMPatch fmBrass;
extern const FMPatch fmLead;
extern const FMPatch fmBell;
extern const FMPatch fmPad;
extern const FMPatch fmKick;

void fm_init(void);
void fm_patch(u8 ch, const FMPatch *p, u8 att);      // att: extra carrier attenuation
void fm_on(u8 ch, u8 note12, u8 oct);                // note12: 0=C .. 11=B
void fm_off(u8 ch);
void fm_freq(u8 ch, u8 note12, u8 oct, s16 fnumOffset);
void fm_rawFreq(u8 ch, u16 fnum, u8 block);
u16  fm_fnumOf(u8 note12);       // base fnum for a semitone - for portamento math
void fm_lfo(u8 speed);                               // 0 = off, 1..8 -> LFO on
void fm_vibrato(u8 ch, u8 fms);                      // per-channel LFO depth 0..7
void fm_allOff(void);

#endif

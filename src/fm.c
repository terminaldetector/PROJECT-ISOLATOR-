#include "fm.h"

// classic NTSC F-number table for block-based note addressing
static const u16 fnums[12] =
{
    644, 681, 722, 765, 810, 858, 910, 964, 1021, 1081, 1146, 1214
};

// ---- the instrument bank -------------------------------------------------
// crafted in the spirit of Konami / Savaged Regime arrangements:
// hard slap bass, punchy brass stabs, singing detunable lead,
// glassy bell for the calm scenes, slow strings pad, FM kick/tom.

const FMPatch fmBass =
{
    .alg = 3, .fb = 6,
    .dtmul = { 0x01, 0x12, 0x01, 0x01 },
    .tl    = { 28,   38,   20,   6 },
    .rsar  = { 0x9F, 0x9F, 0x9F, 0x9F },
    .d1r   = { 0x0E, 0x10, 0x0C, 0x0A },
    .d2r   = { 0x04, 0x04, 0x04, 0x02 },
    .slrr  = { 0x2A, 0x2A, 0x2A, 0x4A },
};

const FMPatch fmBrass =
{
    .alg = 4, .fb = 5,
    .dtmul = { 0x21, 0x11, 0x51, 0x11 },
    .tl    = { 30,   8,    32,   6 },
    .rsar  = { 0x99, 0x9C, 0x99, 0x9C },
    .d1r   = { 0x08, 0x06, 0x08, 0x06 },
    .d2r   = { 0x02, 0x02, 0x02, 0x02 },
    .slrr  = { 0x27, 0x38, 0x27, 0x38 },
};

const FMPatch fmLead =
{
    .alg = 5, .fb = 7,
    .dtmul = { 0x31, 0x11, 0x71, 0x12 },
    .tl    = { 24,   10,   12,   10 },
    .rsar  = { 0x9F, 0x9E, 0x9E, 0x9E },
    .d1r   = { 0x0A, 0x05, 0x05, 0x05 },
    .d2r   = { 0x03, 0x02, 0x02, 0x02 },
    .slrr  = { 0x2A, 0x28, 0x28, 0x28 },
};

const FMPatch fmBell =
{
    .alg = 4, .fb = 3,
    .dtmul = { 0x3E, 0x11, 0x27, 0x12 },
    .tl    = { 40,   14,   44,   12 },
    .rsar  = { 0x9F, 0x9F, 0x9F, 0x9F },
    .d1r   = { 0x10, 0x08, 0x12, 0x08 },
    .d2r   = { 0x06, 0x04, 0x06, 0x04 },
    .slrr  = { 0x2C, 0x17, 0x2C, 0x17 },
};

const FMPatch fmPad =
{
    .alg = 5, .fb = 4,
    .dtmul = { 0x11, 0x31, 0x51, 0x12 },
    .tl    = { 34,   18,   20,   18 },
    .rsar  = { 0x4E, 0x4C, 0x4C, 0x4C },
    .d1r   = { 0x04, 0x02, 0x02, 0x02 },
    .d2r   = { 0x01, 0x01, 0x01, 0x01 },
    .slrr  = { 0x14, 0x25, 0x25, 0x25 },
};

const FMPatch fmKick =
{
    .alg = 4, .fb = 6,
    .dtmul = { 0x00, 0x00, 0x10, 0x00 },
    .tl    = { 26,   4,    30,   6 },
    .rsar  = { 0x9F, 0x9F, 0x9F, 0x9F },
    .d1r   = { 0x14, 0x12, 0x14, 0x12 },
    .d2r   = { 0x08, 0x08, 0x08, 0x08 },
    .slrr  = { 0xFF, 0xF8, 0xFF, 0xF8 },
};

// --------------------------------------------------------------------------

static u8 keyVal(u8 ch)
{
    return (ch < 3) ? ch : (ch + 1);    // 0,1,2, 4,5,6
}

void fm_init(void)
{
    // the Z80 sleeps forever: the 68000 owns the whole sound system
    Z80_requestBus(TRUE);
    YM2612_reset();

    // LFO off, ch3 normal mode, DAC off
    YM2612_writeReg(0, 0x22, 0x00);
    YM2612_writeReg(0, 0x27, 0x00);
    YM2612_writeReg(0, 0x2B, 0x00);

    // all keys off, both speakers on everywhere
    for (u8 ch = 0; ch < 6; ch++)
    {
        YM2612_writeReg(0, 0x28, keyVal(ch));
        YM2612_writeReg(ch / 3, 0xB4 + (ch % 3), 0xC0);
    }
}

void fm_patch(u8 ch, const FMPatch *p, u8 att)
{
    u8 part = ch / 3;
    u8 c = ch % 3;

    // carrier mask per algorithm (register op order 1,3,2,4)
    static const u8 carriers[8] =
    {
        0x08, 0x08, 0x08, 0x08,     // alg 0-3: OP4 only
        0x0A,                       // alg 4: OP2, OP4
        0x0E, 0x0E,                 // alg 5,6: OP2, OP3, OP4
        0x0F,                       // alg 7: all
    };
    // register slots order: index0->OP1(+0), index1->OP3(+4), index2->OP2(+8), index3->OP4(+12)
    // carrier bit mapping for our storage order:
    static const u8 slotBit[4] = { 0x01, 0x04, 0x02, 0x08 };

    for (u8 op = 0; op < 4; op++)
    {
        u8 off = c + (op << 2);
        u8 tl = p->tl[op];
        if (carriers[p->alg] & slotBit[op])
        {
            u16 t = tl + att;
            tl = (t > 127) ? 127 : t;
        }
        YM2612_writeReg(part, 0x30 + off, p->dtmul[op]);
        YM2612_writeReg(part, 0x40 + off, tl);
        YM2612_writeReg(part, 0x50 + off, p->rsar[op]);
        YM2612_writeReg(part, 0x60 + off, p->d1r[op]);
        YM2612_writeReg(part, 0x70 + off, p->d2r[op]);
        YM2612_writeReg(part, 0x80 + off, p->slrr[op]);
        YM2612_writeReg(part, 0x90 + off, 0x00);
    }
    YM2612_writeReg(part, 0xB0 + c, (p->fb << 3) | p->alg);
    YM2612_writeReg(part, 0xB4 + c, 0xC0);
}

void fm_rawFreq(u8 ch, u16 fnum, u8 block)
{
    u8 part = ch / 3;
    u8 c = ch % 3;
    YM2612_writeReg(part, 0xA4 + c, ((block & 7) << 3) | ((fnum >> 8) & 7));
    YM2612_writeReg(part, 0xA0 + c, fnum & 0xFF);
}

void fm_freq(u8 ch, u8 note12, u8 oct, s16 fnumOffset)
{
    s16 f = fnums[note12] + fnumOffset;
    fm_rawFreq(ch, (u16) f, oct);
}

void fm_on(u8 ch, u8 note12, u8 oct)
{
    // retrigger: key off first so the envelope restarts
    YM2612_writeReg(0, 0x28, keyVal(ch));
    fm_freq(ch, note12, oct, 0);
    YM2612_writeReg(0, 0x28, 0xF0 | keyVal(ch));
}

void fm_off(u8 ch)
{
    YM2612_writeReg(0, 0x28, keyVal(ch));
}

void fm_lfo(u8 speed)
{
    YM2612_writeReg(0, 0x22, speed ? (0x08 | (speed - 1)) : 0x00);
}

void fm_vibrato(u8 ch, u8 fms)
{
    YM2612_writeReg(ch / 3, 0xB4 + (ch % 3), 0xC0 | (fms & 7));
}

void fm_allOff(void)
{
    for (u8 ch = 0; ch < 6; ch++) fm_off(ch);
}

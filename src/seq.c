#include "seq.h"
#include "fm.h"
#include "util.h"

// --------------------------------------------------------------------------
// the score: an original track in the spirit of "Astellion"-era Type R,
// harmonies leaning on the sad-keyboard-guy school (m9 / maj7 colors),
// arranged the Konami / Savaged Regime way: every channel always working.
//
// channels: FM0 bass | FM1 leadA | FM2 leadB (detune) | FM3 stabs
//           FM4 arp/pad | FM5 kick+toms+boom
//           PSG0 echo sparkle | PSG2 sub square | PSG3 noise hat/snare
// --------------------------------------------------------------------------

// note byte: (octave << 4) | semitone(C=0..B=11); 0x00 = hold, 0x01 = off
#define N(o, s)  (((o) << 4) | (s))
#define HH 0x00
#define XX 0x01

#define C 0
#define Cs 1
#define D 2
#define Ds 3
#define E 4
#define F 5
#define Fs 6
#define G 7
#define Gs 8
#define A 9
#define As 10
#define B 11

// chord types
#define CH_MIN 0
#define CH_MAJ 1
#define CH_DOM 2

typedef struct { u8 root; u8 type; } Chord;

// A section: Am9 - Fmaj7 - Cadd9 - G  (the heart of the track)
static const Chord progMain[4]  = { {A, CH_MIN}, {F, CH_MAJ}, {C, CH_MAJ}, {G, CH_MAJ} };
// agony: Am - F - Dm - E
static const Chord progAgony[4] = { {A, CH_MIN}, {F, CH_MAJ}, {D, CH_MIN}, {E, CH_DOM} };
// mythic lament: Am - G - F - E
static const Chord progMyth[4]  = { {A, CH_MIN}, {G, CH_MAJ}, {F, CH_MAJ}, {E, CH_DOM} };

static const u8 chordTones[3][5] =
{
    { 0, 3, 7, 10, 14 },    // m7/9
    { 0, 4, 7, 11, 14 },    // maj7/9
    { 0, 4, 7, 10, 13 },    // dom
};

// 4 bars x 16 steps of lead melody (drive sections)
static const u8 leadDrive[64] =
{
    N(5,E),HH,HH,N(5,C),  N(5,D),HH,N(5,E),HH,  N(5,A),HH,HH,N(5,G),  N(5,E),HH,N(5,D),HH,
    N(5,F),HH,HH,N(5,E),  N(5,F),HH,N(5,G),HH,  N(5,A),HH,HH,N(5,F),  N(5,E),HH,N(5,C),HH,
    N(5,E),HH,HH,N(5,D),  N(5,C),HH,N(5,D),N(5,E),  N(5,G),HH,HH,N(5,E),  N(5,D),HH,N(5,C),HH,
    N(4,B),HH,HH,N(5,D),  N(5,G),HH,N(5,A),HH,  N(5,B),HH,N(5,A),N(5,G),  N(5,D),HH,N(4,B),HH,
};

// agony lead: long crying notes, one gesture per bar
static const u8 leadAgony[64] =
{
    N(4,A),HH,HH,HH, HH,HH,HH,HH, N(5,C),HH,HH,HH, N(4,B),HH,HH,HH,
    N(5,C),HH,HH,HH, HH,HH,HH,HH, N(4,A),HH,HH,HH, HH,HH,HH,HH,
    N(5,D),HH,HH,HH, HH,HH,N(5,E),HH, HH,HH,HH,HH, N(5,F),HH,HH,HH,
    N(5,E),HH,HH,HH, HH,HH,HH,HH, HH,HH,HH,HH, N(4,B),HH,HH,HH,
};

// mythic hymn: descending cantus over the lament bass
static const u8 leadMyth[64] =
{
    N(5,A),HH,HH,HH, HH,HH,HH,HH, N(5,E),HH,HH,HH, N(5,C),HH,HH,HH,
    N(5,G),HH,HH,HH, HH,HH,HH,HH, N(5,B),HH,HH,HH, N(5,G),HH,HH,HH,
    N(5,F),HH,HH,HH, HH,HH,HH,HH, N(5,A),HH,HH,HH, N(5,C),HH,HH,HH,
    N(5,E),HH,HH,HH, N(5,D),HH,HH,HH, N(4,B),HH,HH,HH, N(4,Gs),HH,HH,HH,
};

// bass groove as semitone offsets from the chord root (oct 2), 0xFF = rest
static const u8 bassDrive[16] = { 0,0,12,0, 0,12,0,0, 0,0,12,0, 7,7,10,10 };
static const u8 bassCalm[16]  = { 0,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 12,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF };
static const u8 bassMyth[16]  = { 0,0xFF,0xFF,0xFF, 12,0xFF,0xFF,0xFF, 0,0xFF,0xFF,0xFF, 12,0xFF,7,0xFF };

// drum masks per section: bit set = event on that step
static const u16 kickDrive = 0x1111;    // four on the floor
static const u16 kickHard  = 0x5555;    // doubled - the hardcore
static const u16 snareMask = 0x1010;    // backbeat 4 & 12
static const u16 stabDrive = 0x4210;    // syncopated brass hits
static const u16 stabHard  = 0x4444;

// --------------------------------------------------------------------------

static u8  section = SEC_OFF;
static u16 frameInStep, stepLen;
static u16 step, bar;
static u16 kickFlag, snareFlag, downbeatFlag;
static u16 kickPhase = 99, tomPhase = 99;
static u16 boomTimer;
static u16 snarePhase = 99, hatPhase = 99;
static u8  leadHist[8];

static const Chord *prog;
static const u8 *leadPat;
static const u8 *bassPat;
static u16 kickMask, stabMask;

static void loadSectionPatches(void)
{
    switch (section)
    {
        case SEC_CALM:
            fm_patch(0, &fmBass, 8);
            fm_patch(1, &fmBell, 4);
            fm_patch(2, &fmBell, 12);
            fm_patch(3, &fmPad,  8);
            fm_patch(4, &fmBell, 8);
            fm_patch(5, &fmKick, 6);
            fm_lfo(0);
            break;
        case SEC_AGONY:
            fm_patch(0, &fmBass, 6);
            fm_patch(1, &fmLead, 2);
            fm_patch(2, &fmLead, 8);
            fm_patch(3, &fmPad,  4);
            fm_patch(4, &fmPad,  10);
            fm_patch(5, &fmKick, 0);
            fm_lfo(4);
            fm_vibrato(1, 5);           // the lead cries
            fm_vibrato(2, 6);
            break;
        case SEC_MYTHIC:
            fm_patch(0, &fmBass, 2);
            fm_patch(1, &fmBrass, 2);
            fm_patch(2, &fmBrass, 8);
            fm_patch(3, &fmPad,  2);
            fm_patch(4, &fmBell, 6);
            fm_patch(5, &fmKick, 0);
            fm_lfo(3);
            fm_vibrato(1, 2);
            break;
        default:                        // DRIVE / HARD
            fm_patch(0, &fmBass, 0);
            fm_patch(1, &fmLead, 4);
            fm_patch(2, &fmLead, 9);
            fm_patch(3, &fmBrass, 6);
            fm_patch(4, &fmBell, 10);
            fm_patch(5, &fmKick, 0);
            fm_lfo(0);
            break;
    }
}

void seq_init(void)
{
    fm_init();
    section = SEC_OFF;
    stepLen = 6;
    step = 0;
    bar = 0;
    frameInStep = 0;
}

void seq_setSection(u8 sec)
{
    if (sec == section) return;
    section = sec;

    fm_allOff();
    PSG_setEnvelope(0, 15);
    PSG_setEnvelope(1, 15);
    PSG_setEnvelope(2, 15);
    PSG_setEnvelope(3, 15);

    if (sec == SEC_OFF) return;

    // tempo per section: 150 BPM 16ths, half-time feel for the slow ones
    stepLen = (sec == SEC_AGONY || sec == SEC_MYTHIC) ? 9 : 6;

    switch (sec)
    {
        case SEC_AGONY:  prog = progAgony; leadPat = leadAgony; bassPat = bassCalm;
                         kickMask = 0x0001; stabMask = 0; break;
        case SEC_MYTHIC: prog = progMyth;  leadPat = leadMyth;  bassPat = bassMyth;
                         kickMask = 0x0101; stabMask = 0x0010; break;
        case SEC_CALM:   prog = progMain;  leadPat = leadDrive; bassPat = bassCalm;
                         kickMask = 0; stabMask = 0; break;
        case SEC_HARD:   prog = progMain;  leadPat = leadDrive; bassPat = bassDrive;
                         kickMask = kickHard; stabMask = stabHard; break;
        default:         prog = progMain;  leadPat = leadDrive; bassPat = bassDrive;
                         kickMask = kickDrive; stabMask = stabDrive; break;
    }

    loadSectionPatches();
    step = 0;
    bar = 0;
    frameInStep = 0;
}

void seq_boom(void)
{
    boomTimer = 40;
    fm_patch(5, &fmKick, 0);
}

// ---- per-step event dispatch ---------------------------------------------

static void fireStep(void)
{
    u16 barIdx = bar & 3;
    const Chord *ch = &prog[barIdx];
    u16 stepBit = 1 << step;
    u16 patIdx = (barIdx << 4) | step;

    // BASS (FM0) + sub square (PSG2)
    u8 bofs = bassPat[step];
    if (bofs != 0xFF && section != SEC_CALM)
    {
        u8 semi = ch->root + (bofs % 12);
        u8 oct = 2 + (bofs / 12) + (semi > 11 ? 1 : 0);
        fm_on(0, semi % 12, oct);
        PSG_setFrequency(2, 55 + ch->root * 3);
        PSG_setEnvelope(2, (section == SEC_CALM) ? 15 : 9);
    }
    else if (section == SEC_CALM && bofs != 0xFF)
        fm_on(0, ch->root, 2);

    // LEAD (FM1 + FM2 detuned unison), echo history for PSG0
    u8 ln = leadPat[patIdx];
    if (ln == XX)
    {
        fm_off(1);
        fm_off(2);
    }
    else if (ln != HH)
    {
        u8 semi = ln & 15, oct = ln >> 4;
        // calm plays the melody sparsely, as distant bells
        if (section != SEC_CALM || (step & 3) == 0)
        {
            fm_on(1, semi, oct);
            fm_freq(2, semi, oct, (section == SEC_AGONY) ? 10 : 5);
            YM2612_writeReg(0, 0x28, 0xF0 | 2);
        }
    }
    leadHist[step & 7] = (ln > 1) ? ln : leadHist[(step - 1) & 7];

    // PSG0: sparkling echo of the lead, 3 steps late, one octave up
    u8 echo = leadHist[(step - 3) & 7];
    if (echo > 1 && section != SEC_AGONY)
    {
        u16 f = 440;
        u8 s = echo & 15, o = (echo >> 4) + 1;
        // quick integer note->Hz: A4=440 reference, rough equal temper walk
        static const u16 hz[12] = { 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494 };
        f = hz[s] << (o > 4 ? (o - 4) : 0);
        if (o < 4) f >>= (4 - o);
        PSG_setFrequency(0, f);
        PSG_setEnvelope(0, 11);
    }
    else PSG_setEnvelope(0, 14);

    // STABS (FM3): chord hits; MYTHIC holds choral chords instead
    if (section == SEC_MYTHIC || section == SEC_AGONY || section == SEC_CALM)
    {
        if (step == 0)
        {
            fm_on(3, ch->root, 3);
            // FM4 holds the color tone (7th/9th) as a second choral voice
            u8 color = chordTones[ch->type][3];
            fm_on(4, (ch->root + color) % 12, 4);
        }
    }
    else
    {
        if (stabMask & stepBit)
        {
            fm_on(3, ch->root, 4);
        }
        // ARP (FM4): 16th chord-tone climb
        u8 tone = chordTones[ch->type][step & 3];
        fm_on(4, (ch->root + tone) % 12, 4 + ((ch->root + tone) > 11 ? 1 : 0));
    }

    // DRUMS
    if (!boomTimer && (kickMask & stepBit))
    {
        fm_on(5, C, 2);
        kickPhase = 0;
        kickFlag = TRUE;
    }
    // mythic toms roll into each bar
    if (section == SEC_MYTHIC && step >= 13 && (bar & 1))
    {
        fm_on(5, G, 2);
        tomPhase = 0;
    }
    if (snareMask & stepBit && (section == SEC_DRIVE || section == SEC_HARD))
    {
        snarePhase = 0;
        snareFlag = TRUE;
    }
    else if (section == SEC_DRIVE || section == SEC_HARD || section == SEC_MYTHIC)
        hatPhase = 0;
}

// ---- per-frame continuous processing -------------------------------------

void seq_tick(void)
{
    kickFlag = FALSE;
    snareFlag = FALSE;
    downbeatFlag = FALSE;

    if (section == SEC_OFF)
    {
        if (boomTimer) goto fx;
        return;
    }

    if (frameInStep == 0)
    {
        if (step == 0) downbeatFlag = TRUE;
        fireStep();
    }
    if (++frameInStep >= stepLen)
    {
        frameInStep = 0;
        if (++step >= 16)
        {
            step = 0;
            bar++;
        }
    }

fx:
    // kick pitch drop - the FM thump
    if (kickPhase < 6)
    {
        fm_rawFreq(5, 620 - kickPhase * 85, 2);
        kickPhase++;
    }
    if (tomPhase < 8)
    {
        fm_rawFreq(5, 700 - tomPhase * 40, 3);
        tomPhase++;
    }

    // boom: long pitch fall + noise wash, overrides the kick
    if (boomTimer)
    {
        boomTimer--;
        fm_rawFreq(5, 200 + boomTimer * 16, 1);
        if ((boomTimer & 3) == 0) YM2612_writeReg(0, 0x28, 0xF0 | 6);
        PSG_setNoise(1, 2);
        PSG_setEnvelope(3, 4 + ((40 - boomTimer) >> 3));
        return;
    }

    // snare / hat share the noise channel, snare wins
    if (snarePhase < 5)
    {
        PSG_setNoise(1, 1);
        PSG_setEnvelope(3, 5 + snarePhase * 2);
        snarePhase++;
    }
    else if (hatPhase < 2)
    {
        PSG_setNoise(1, 0);
        PSG_setEnvelope(3, 10 + hatPhase * 3);
        hatPhase++;
    }
    else
        PSG_setEnvelope(3, 15);
}

u16 seq_step(void)       { return step; }
u16 seq_bar(void)        { return bar; }
u16 seq_isKick(void)     { return kickFlag; }
u16 seq_isSnare(void)    { return snareFlag; }
u16 seq_isDownbeat(void) { return downbeatFlag; }

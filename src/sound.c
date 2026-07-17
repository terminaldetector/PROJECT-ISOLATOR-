#include "sound.h"
#include "util.h"

// fully procedural PSG score: a seeded pattern over a minor pentatonic scale
static const u16 scale[8] = { 110, 131, 147, 175, 196, 220, 262, 294 };

static u8  mood = SND_OFF;
static u16 boomTimer = 0;
static u8  pattern[16];

void snd_init(void)
{
    for (u16 i = 0; i < 16; i++) pattern[i] = rnd_range(8);
    PSG_reset();
}

void snd_setMood(u8 m)
{
    mood = m;
    if (mood == SND_OFF)
    {
        PSG_setEnvelope(0, 15);
        PSG_setEnvelope(1, 15);
        PSG_setEnvelope(2, 15);
        PSG_setEnvelope(3, 15);
    }
}

void snd_boom(void)
{
    boomTimer = 90;
}

void snd_update(u32 frame)
{
    u16 step = (frame >> 4) & 15;

    if (boomTimer)
    {
        boomTimer--;
        // white noise rumble with decay + low tone drop
        PSG_setNoise(1, 2);
        PSG_setEnvelope(3, 15 - (boomTimer >> 3));
        PSG_setFrequency(2, 55 + boomTimer);
        PSG_setEnvelope(2, 15 - (boomTimer >> 3));
        return;
    }

    if (mood == SND_OFF) return;

    if (mood == SND_AGONY)
    {
        // y2k elegy: slow descending bass with a pitch slide, a detuned
        // drone beating against it, trembling lead, hardcore noise kicks
        static const u16 agonyBass[8] = { 110, 98, 87, 82, 73, 65, 58, 55 };
        static const u16 agonyLead[8] = { 220, 233, 262, 220, 349, 330, 294, 233 };

        u16 bstep = (frame >> 5) & 7;
        u16 slide = (frame & 31) >> 2;
        PSG_setFrequency(0, agonyBass[bstep] - slide);
        PSG_setEnvelope(0, 4 + ((frame >> 2) & 3));

        // drone a hair off the root - constant slow beating
        PSG_setFrequency(2, 110 + ((frame >> 7) & 1));
        PSG_setEnvelope(2, 9);

        // sparse trembling lead, only on alternating half-bars
        u16 lstep = (frame >> 4) & 7;
        PSG_setFrequency(1, agonyLead[lstep]);
        PSG_setEnvelope(1, ((frame >> 3) & 1) ? (7 + (frame & 7)) : 15);

        // kick doubles up in the hardcore sections
        u16 kickMask = (frame & 2048) ? 7 : 15;
        if ((frame & kickMask) == 0)
        {
            PSG_setNoise(0, 2);
            PSG_setEnvelope(3, 4);
        }
        else
            PSG_setEnvelope(3, 12 + (frame & 3));
        return;
    }

    // channel 0: bass line, new note every 16 frames, fast decay
    u16 decay = frame & 15;
    PSG_setFrequency(0, scale[pattern[step]]);
    PSG_setEnvelope(0, 6 + (decay >> 1));

    if (mood == SND_DRIVE)
    {
        // channel 1: arpeggio one octave up, every 8 frames
        u16 fast = (frame >> 3) & 15;
        PSG_setFrequency(1, scale[pattern[fast]] << 1);
        PSG_setEnvelope(1, 8 + ((frame & 7)));

        // channel 3: noise hat tick
        if ((frame & 7) == 0)
        {
            PSG_setNoise(1, 0);
            PSG_setEnvelope(3, 10);
        }
        else
            PSG_setEnvelope(3, 13 + (frame & 3));
    }
    else
    {
        PSG_setEnvelope(1, 15);
        PSG_setEnvelope(3, 15);
    }
}

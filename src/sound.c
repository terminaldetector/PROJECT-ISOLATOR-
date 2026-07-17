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
    PSG_init();
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

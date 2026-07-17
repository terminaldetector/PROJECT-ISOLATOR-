#include "sound.h"
#include "seq.h"
#include "fm.h"
#include "util.h"

// facade over the FM engine: scenes speak in moods,
// the sequencer turns moods into music

static u8 mood = SND_OFF;

void snd_init(void)
{
    seq_init();
}

void snd_setMood(u8 m)
{
    if (m == mood) return;
    mood = m;

    switch (m)
    {
        case SND_CALM:   seq_setSection(SEC_CALM);   break;
        case SND_DRIVE:  seq_setSection(SEC_DRIVE);  break;
        case SND_HARD:   seq_setSection(SEC_HARD);   break;
        case SND_AGONY:  seq_setSection(SEC_AGONY);  break;
        case SND_MYTHIC: seq_setSection(SEC_MYTHIC); break;
        default:         seq_setSection(SEC_OFF);    break;
    }
}

void snd_boom(void)
{
    seq_boom();
}

void snd_update(u32 frame)
{
    (void) frame;
    seq_tick();
}

#ifndef SOUND_H
#define SOUND_H

#include <genesis.h>

#define SND_OFF     0
#define SND_CALM    1
#define SND_DRIVE   2

void snd_init(void);
void snd_setMood(u8 mood);
void snd_boom(void);
void snd_update(u32 frame);

#endif

#include <genesis.h>
#include "util.h"
#include "timeline.h"
#include "sound.h"

// SYNTAXIS: FRACTAL GENESIS
// a fully procedural tech demo for the Sega Mega Drive / Genesis
// no precomputed levels - only generated scenes

int main(bool hardReset)
{
    (void) hardReset;

    util_init();
    rnd_seed(0xA5F3);
    snd_init();

    JOY_init();

    timeline_run();
    return 0;
}

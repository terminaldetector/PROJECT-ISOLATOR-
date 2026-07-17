#include <genesis.h>
#include "scenes.h"
#include "timeline.h"
#include "util.h"
#include "fxpal.h"
#include "sound.h"

// scene 1: fake neural compiler boot screen

static const char *bootLines[] =
{
    "NEURAL ROM COMPILER V0.1",
    "(C) 2026 SYNTAXIS LABS",
    "",
    "TARGET   : SEGA MEGA DRIVE",
    "CPU      : MC68000 @ 7.67 MHZ",
    "VDP      : 315-5313  OK",
    "MEMORY   : 64K + 64K  OK",
    "",
    "SEED     : 0XA5F3",
    "MODE     : REALTIME SYNTHESIS",
    "",
    "GENERATING WORLD...",
};

#define NUM_LINES   12
#define CHAR_RATE   2       // frames per character
#define BAR_Y       16
#define BAR_LEN     28

static u16 shown;           // characters revealed so far
static u16 total;           // total characters
static u16 barFill;

void boot_init(void)
{
    demo_enterTiles();

    // green terminal on black
    PAL_setColor(0, 0x0000);
    PAL_setColor(15, VCOL(1, 7, 2));
    VDP_setTextPalette(PAL0);
    VDP_setBackgroundColor(0);

    shown = 0;
    barFill = 0;
    total = 0;
    for (u16 i = 0; i < NUM_LINES; i++) total += strlen(bootLines[i]);

    snd_setMood(SND_OFF);
}

void boot_update(u16 t)
{
    // typewriter reveal
    u16 want = t / CHAR_RATE;
    if (want > total) want = total;

    while (shown < want)
    {
        // find line/column of character number 'shown'
        u16 n = shown;
        u16 line = 0;
        while (line < NUM_LINES && n >= strlen(bootLines[line]))
        {
            n -= strlen(bootLines[line]);
            line++;
        }
        if (line < NUM_LINES)
        {
            char c[2] = { bootLines[line][n], 0 };
            VDP_drawText(c, 2 + n, 2 + line);
            // blip on every few characters
            if ((shown & 7) == 0)
            {
                PSG_setFrequency(1, 880 + (rnd() & 255));
                PSG_setEnvelope(1, 9);
            }
        }
        shown++;
    }
    if ((t & 3) == 0) PSG_setEnvelope(1, 15);

    // progress bar once the text is done
    if (shown >= total)
    {
        u16 fill = (t > 300) ? ((t - 300) / 4) : 0;
        if (fill > BAR_LEN) fill = BAR_LEN;
        while (barFill < fill)
        {
            VDP_drawText("#", 2 + barFill, BAR_Y);
            barFill++;
        }
        VDP_drawText("[", 1, BAR_Y);
        VDP_drawText("]", 2 + BAR_LEN, BAR_Y);
    }

    // blinking cursor
    VDP_drawText(((t >> 4) & 1) ? "_" : " ", 2 + barFill, BAR_Y + 2);

    // scrolling hex garbage at the bottom: the "network" is thinking
    if ((t & 3) == 0 && t > 60)
    {
        static const char hex[] = "0123456789ABCDEF";
        char buf[37];
        for (u16 i = 0; i < 36; i++) buf[i] = hex[rnd() & 15];
        buf[36] = 0;
        VDP_drawText(buf, 2, 26);
    }

    // terminal green flickers slightly
    if ((t & 15) == 0)
        PAL_setColor(15, VCOL(1, 6 + (rnd() & 1), 2));
}

#include "timeline.h"
#include "scenes.h"
#include "sound.h"
#include "util.h"
#include "fxpal.h"

u32 g_frame = 0;

static u16 bmpOn = FALSE;

static const Scene scenes[] =
{
    { boot_init,       boot_update,       480 },   // NEURAL ROM COMPILER
    { birth_init,      birth_update,      900 },   // grid -> triangle -> eye
    { city_init,       city_update,       960 },   // wireframe city build-up
    { fractal_init,    fractal_update,    720 },   // sierpinski morph
    { tunnel_init,     tunnel_update,     720 },   // polygon tunnel
    { landscape_init,  landscape_update,  900 },   // pseudo mode7 landscape
    { metropolis_init, metropolis_update, 840 },   // vector megapolis scroller
    { swarm_init,      swarm_update,      780 },   // 80 sprites storm
    { mushroom_init,   mushroom_update,   660 },   // procedural nuke
    { rain_init,       rain_update,       600 },   // digital rain
    { finale_init,     finale_update,     980 },   // breakdown + credits
    { flower_init,     flower_update,    1150 },   // falling flower epilogue
};

#define NUM_SCENES  (sizeof(scenes) / sizeof(Scene))

void demo_enterTiles(void)
{
    if (bmpOn)
    {
        BMP_end();
        bmpOn = FALSE;
    }

    SYS_disableInts();
    VDP_setHInterrupt(FALSE);
    VDP_setScreenWidth320();
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_clearSprites();
    VDP_updateSprites(1, DMA);
    VDP_setTextPlane(BG_A);
    SYS_enableInts();

    fx_allBlack();
}

void demo_enterBMP(void)
{
    demo_enterTiles();
    BMP_init(TRUE, BG_A, PAL1, FALSE);
    bmpOn = TRUE;
}

void timeline_run(void)
{
    u16 cur = 0;
    u16 t = 0;
    u16 prevJoy = 0;

    scenes[0].init();

    while (TRUE)
    {
        scenes[cur].update(t);
        snd_update(g_frame);

        // START skips to the next scene (handy on hardware / emulator)
        u16 joy = JOY_readJoypad(JOY_1);
        u16 skip = (joy & BUTTON_START) && !(prevJoy & BUTTON_START);
        prevJoy = joy;

        SYS_doVBlankProcess();
        g_frame++;
        t++;

        if (t >= scenes[cur].duration || skip)
        {
            t = 0;
            cur++;
            if (cur >= NUM_SCENES)
            {
                cur = 0;
                // every loop regenerates a different world
                rnd_seed((u16) g_frame ^ 0x1234);
            }
            scenes[cur].init();
        }
    }
}

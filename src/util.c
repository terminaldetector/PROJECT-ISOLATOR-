#include "util.h"

s16 g_sin[256];

static u16 seed = 0xA5F3;

// Bhaskara I integer approximation - no float, no precomputed table:
// sin(x deg) ~= 4x(180-x) / (40500 - x(180-x))
void util_init(void)
{
    for (u16 a = 0; a < 128; a++)
    {
        s32 x = ((s32) a * 180) >> 7;           // angle in degrees, 0..180
        s32 p = x * (180 - x);
        s32 v = (p << 10) / (40500 - p);        // 4*p*256 / (40500-p)
        g_sin[a] = (s16) v;
        g_sin[a + 128] = (s16) -v;
    }
}

void rnd_seed(u16 s)
{
    seed = s ? s : 0xA5F3;
}

u16 rnd(void)
{
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return seed;
}

u16 rnd_range(u16 n)
{
    return rnd() % n;
}

u16 isqrt32(u32 n)
{
    u32 res = 0;
    u32 bit = 1UL << 30;         // highest even power of 4 <= max u32
    while (bit > n) bit >>= 2;
    while (bit != 0)
    {
        if (n >= res + bit)
        {
            n -= res + bit;
            res = (res >> 1) + bit;
        }
        else
            res >>= 1;
        bit >>= 2;
    }
    return (u16) res;
}

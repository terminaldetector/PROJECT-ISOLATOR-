#ifndef SCENES_H
#define SCENES_H

#include <genesis.h>

void boot_init(void);       void boot_update(u16 t);
void birth_init(void);      void birth_update(u16 t);
void city_init(void);       void city_update(u16 t);
void fractal_init(void);    void fractal_update(u16 t);
void tunnel_init(void);     void tunnel_update(u16 t);
void landscape_init(void);  void landscape_update(u16 t);
void metropolis_init(void); void metropolis_update(u16 t);
void swarm_init(void);      void swarm_update(u16 t);
void mushroom_init(void);   void mushroom_update(u16 t);
void rain_init(void);       void rain_update(u16 t);
void finale_init(void);     void finale_update(u16 t);
void flower_init(void);     void flower_update(u16 t);

void tree_init(void);       void tree_update(u16 t);
void aleph_init(void);      void aleph_update(u16 t);
void myth_init(void);       void myth_update(u16 t);

// shared by swarm / metropolis / finale: procedural triangle sprite tiles
// sizes 8/16/24/32 px, returns base VRAM tile index for each of the 4 sizes
void gen_triangleSprites(u16 baseTile, u16 *tileIndex);

// shared recursive branching (fractal scene + tree of life)
void frac_branch(s16 x, s16 y, u16 ang, s16 len, u8 depth, u8 grow, u16 sway);

#endif

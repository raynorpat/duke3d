/* Private declarations shared by the engine implementation modules. */
#ifndef ENGINE_INTERNAL_H
#define ENGINE_INTERNAL_H

#include <stdint.h>

void engine_loadtables(void);
int32_t engine_render_sqrt(uint32_t value);
int32_t engine_recip(int32_t value);
int32_t getclipmask(int i1, int i2, int i3, int i4);
extern short radarang[1280];
extern int32_t pow2long[32];
extern char britable[16][64];

#endif

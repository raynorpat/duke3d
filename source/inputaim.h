#ifndef DUKE3D_INPUTAIM_H
#define DUKE3D_INPUTAIM_H
#include <stdint.h>

/* Q16 camera units, kept outside serialized player and input structures. */
typedef struct { int64_t yaw, horizon; } InputAim;
/* scale is the renderer focal length measured in horizon units. */
void INPUT_AddLook(InputAim *aim, int64_t yaw, int64_t pitch, int32_t horizon, double scale);
/* Rates are Q16 camera units per second; elapsed time is in microseconds. */
void INPUT_AddRate(InputAim *aim, int64_t yaw, int64_t pitch, uint32_t usec, int32_t horizon, double scale);
void INPUT_ConsumeLook(InputAim *aim, int turnscale, int32_t *turn, int32_t *look);
#endif

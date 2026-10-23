#include "inputaim.h"
#include <math.h>

/*
================
INPUT_AddLook

Accumulate Q16 yaw and apply pitch in angle space before converting back
to a Q16 horizon remainder around the supplied integer horizon.
================
*/
void INPUT_AddLook(InputAim *aim, int64_t yaw, int64_t pitch, int32_t horizon, double scale)
{
    double h, angle;
    aim->yaw += yaw;
    if (pitch == 0) return;
    h = horizon + aim->horizon / 65536.0;
    /* Horizon is a shear coordinate. Convert it to an angle so equal mouse
     * motion produces equal angular pitch at different view elevations. */
    angle = atan((h - 100.0) / scale) + pitch / (65536.0 * scale);
    if (angle > atan(199.0 / scale)) h = 299.0;
    else if (angle < atan(-199.0 / scale)) h = -99.0;
    else h = 100.0 + scale * tan(angle);
    aim->horizon = (int64_t)llround((h - horizon) * 65536.0);
}

/*
================
INPUT_AddRate

Integrate Q16 look rates per second over a microsecond interval
and pass the resulting displacement through the same pitch conversion.
================
*/
void INPUT_AddRate(InputAim *aim, int64_t yaw, int64_t pitch, uint32_t usec, int32_t horizon, double scale)
{
    INPUT_AddLook(aim, yaw * usec / 1000000,
                  pitch * usec / 1000000, horizon, scale);
}

/*
================
INPUT_ConsumeLook

Extract bounded turn and even-valued horizon commands from Q16 look.
Subtract only the represented motion so fractional and clipped excess remains.
================
*/
void INPUT_ConsumeLook(InputAim *aim, int turnscale, int32_t *turn, int32_t *look)
{
    /* Signed division truncates toward zero, preserving symmetric small
     * positive and negative remainders instead of biasing them by a shift. */
    int64_t t = aim->yaw / (65536 * turnscale);
    int64_t h = aim->horizon / 65536;
    if (t > 127) t = 127;
    if (t < -127) t = -127;
    if (h > 63) h = 63;
    if (h < -63) h = -63;
    *turn = (int32_t)t;
    *look = (int32_t)h * 2;
    aim->yaw -= t * turnscale * 65536;
    aim->horizon -= h * 65536;
}

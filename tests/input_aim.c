/* Fractional look is independent of tick quantization and frame partitioning. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "inputaim.h"

static int failures;
static void check(const char *name, int ok)
{
    if (!ok) { printf("FAIL %s\n", name); failures++; }
}
int main(void)
{
    InputAim a = {0}, b = {0};
    int32_t turn, look;
    int i;
    INPUT_AddLook(&a, 16384, 0, 100, 128.0);
    INPUT_ConsumeLook(&a, 2, &turn, &look);
    check("sub-unit input survives tick", turn == 0 && a.yaw == 16384);
    INPUT_AddLook(&a, -16384, 0, 100, 128.0);
    check("opposite slow input cancels", a.yaw == 0);
    for (i = 0; i < 8; i++) INPUT_AddLook(&a, 16384, 0, 100, 128.0);
    INPUT_ConsumeLook(&a, 2, &turn, &look);
    check("fractions reach simulation", turn == 1 && a.yaw == 0);
    INPUT_AddLook(&a, -131072, -65536, 100, 128.0);
    INPUT_ConsumeLook(&a, 2, &turn, &look);
    check("both negative axes contribute symmetrically", turn == -1 && look == -2);
    a.yaw = a.horizon = 0;
    INPUT_AddLook(&a, 0, 65536, 100, 128.0);
    INPUT_AddLook(&b, 0, 65536, 250, 128.0);
    check("equal pitch angles at different horizons",
        fabs(atan((100 + a.horizon/65536.0 - 100)/128.0) -
             (atan((250 + b.horizon/65536.0 - 100)/128.0) - atan(150/128.0))) < 0.000001);
    a.yaw = a.horizon = b.yaw = b.horizon = 0;
    for (i = 0; i < 100; i++) INPUT_AddRate(&a, 90*65536, -16*65536, 10000, 100, 128.0);
    for (i = 0; i < 20; i++) INPUT_AddRate(&b, 90*65536, -16*65536, 50000, 100, 128.0);
    check("stick yaw independent of frame rate", llabs(a.yaw-b.yaw) < 100);
    check("stick pitch independent of frame rate", llabs(a.horizon-b.horizon) < 100);
    a.yaw = a.horizon = 0;
    INPUT_AddLook(&a, 0, 100000000, 299, 128.0);
    check("pitch clamps without accumulating excess", a.horizon == 0);
    return failures != 0;
}

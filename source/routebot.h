#ifndef DUKE3D_ROUTEBOT_H
#define DUKE3D_ROUTEBOT_H
/* duke3d.h supplies input and the fixed-width coordinate types. */
int routebot_load(const char *path);
/* 1: one input, 0: route complete, -1: waypoint timed out. */
int routebot_next(int32_t x,int32_t y,short angle,input *out);
#endif

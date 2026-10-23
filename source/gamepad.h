#ifndef DUKE3D_GAMEPAD_H
#define DUKE3D_GAMEPAD_H

#include <stdint.h>

extern uint32_t GAMEPAD_ButtonState1, GAMEPAD_ButtonState2;

void GAMEPAD_Init(void);
void GAMEPAD_Shutdown(void);
void GAMEPAD_Poll(void);
void GAMEPAD_ClearButton(int action);
int GAMEPAD_Connected(void);
void GAMEPAD_AddInput(int running, int32_t *forward, int32_t *strafe,
                     int32_t *turn, int32_t *look);

#endif

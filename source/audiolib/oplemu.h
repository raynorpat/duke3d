/*
   OPLEMU: an emulated OPL3 (DOSBox's DBOPL) for AL_MIDI, with a stand-in
   for TASK_MAN that runs MIDI's player task in step with the output.
*/

#ifndef __OPLEMU_H
#define __OPLEMU_H
#include <stdint.h>

void OPLEMU_Init( int rate );
void OPLEMU_WriteReg( int reg, int data );
void OPLEMU_SetVolume( int volume );
int  OPLEMU_GetVolume( void );
void OPLEMU_Render( int16_t *out, int frames, int channels );

#endif

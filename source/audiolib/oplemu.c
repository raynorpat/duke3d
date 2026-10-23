/*
   OPLEMU: an emulated OPL3 (DOSBox's DBOPL) for AL_MIDI, with a stand-in
   for TASK_MAN that runs MIDI's player task in step with the output.

   On DOS, TASK_MAN called the player from the timer interrupt and AL_MIDI
   wrote straight to the card. Here the player is called between blocks of
   rendered samples instead, at the rate MIDI_SetTempo gives it, so the
   timing is exact however the output is buffered. The caller holds the
   audio lock (DisableInterrupts) around everything.
*/

#include <stdint.h>
#include <stddef.h>
#include "task_man.h"
#include "dbopl.h"
#include "oplemu.h"

#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )

static Chip     OPLEMU_Chip;
static int      OPLEMU_Rate;
static int      OPLEMU_Volume = 255;

// MIDI is the only user, and schedules one task at a time.
static task     OPLEMU_Task;
static uint32_t OPLEMU_FramesToTick;
static uint32_t OPLEMU_TickRemainder;

volatile int TS_InInterrupt = FALSE;

/*
================
OPLEMU_Init

Initialize the local OPL chip at the output sample rate, creating shared
emulator tables only on the first call.
================
*/
void OPLEMU_Init
   (
   int rate
   )

   {
   static int tables;

   if ( !tables )
      {
      DBOPL_InitTables();
      tables = TRUE;
      }

   Chip__Chip( &OPLEMU_Chip );
   Chip__Setup( &OPLEMU_Chip, rate );
   OPLEMU_Rate = rate;
   }


// The FM volume on the card's mixer, as MIDI sets it on a Sound Blaster
// Pro or 16. It scales the output linearly, like MV_SetVolume does the
// digitized sound, rather than in the mixer's 2 dB steps.
void OPLEMU_SetVolume
   (
   int volume
   )

   {
   OPLEMU_Volume = volume;
   }


int OPLEMU_GetVolume
   (
   void
   )

   {
   return( OPLEMU_Volume );
   }


void OPLEMU_WriteReg
   (
   int reg,
   int data
   )

   {
   Chip__WriteReg( &OPLEMU_Chip, reg, data );
   }


/*
================
OPLEMU_Generate

Render bounded DBOPL blocks into mono/stereo signed 16-bit output.
Apply music volume and saturate before storing the samples.
================
*/
static void OPLEMU_Generate
   (
   int16_t *out,
   int frames,
   int channels
   )

   {
   Bit32s mix[ 512 * 2 ];
   int    count;
   int    i;
   int    stereo;
   int32_t left;
   int32_t right;

   while( frames > 0 )
      {
      count = ( frames < 512 ) ? frames : 512;

      stereo = OPLEMU_Chip.opl3Active;
      if ( stereo )
         {
         Chip__GenerateBlock3( &OPLEMU_Chip, count, mix );
         }
      else
         {
         Chip__GenerateBlock2( &OPLEMU_Chip, count, mix );
         }

      for( i = 0; i < count; i++ )
         {
         left  = stereo ? mix[ i * 2 ] : mix[ i ];
         right = stereo ? mix[ i * 2 + 1 ] : mix[ i ];

         if ( channels == 1 )
            {
            left = ( left + right ) / 2;
            }

         left  = left * OPLEMU_Volume / 255;
         right = right * OPLEMU_Volume / 255;

         if ( left > 32767 ) left = 32767;
         if ( left < -32768 ) left = -32768;
         if ( right > 32767 ) right = 32767;
         if ( right < -32768 ) right = -32768;

         *out++ = left;
         if ( channels == 2 )
            {
            *out++ = right;
            }
         }

      frames -= count;
      }
   }


/*
================
OPLEMU_Render

Render samples up to each scheduled MIDI tick, service that tick,
and continue until the requested output frames have been produced.
================
*/
void OPLEMU_Render
   (
   int16_t *out,
   int frames,
   int channels
   )

   {
   uint32_t count;
   uint64_t span;

   while( frames > 0 )
      {
      count = frames;

      if ( OPLEMU_Task.active && ( OPLEMU_Task.rate > 0 ) )
         {
         if ( OPLEMU_FramesToTick == 0 )
            {
            TS_InInterrupt = TRUE;
            OPLEMU_Task.TaskService( &OPLEMU_Task );
            TS_InInterrupt = FALSE;

            // The task may have changed its rate or ended.
            if ( !OPLEMU_Task.active || ( OPLEMU_Task.rate <= 0 ) )
               {
               continue;
               }

            /* Divide sample rate by task rate with a carried remainder, distributing
             * fractional frames across ticks instead of rounding every period. */
            span = ( uint64_t )OPLEMU_Rate + OPLEMU_TickRemainder;
            OPLEMU_FramesToTick  = span / OPLEMU_Task.rate;
            OPLEMU_TickRemainder = span % OPLEMU_Task.rate;
            if ( OPLEMU_FramesToTick == 0 )
               {
               continue;
               }
            }

         /* Do not render across a tick boundary with the old register state;
          * service the MIDI task before generating the following samples. */
         if ( count > OPLEMU_FramesToTick )
            {
            count = OPLEMU_FramesToTick;
            }
         OPLEMU_FramesToTick -= count;
         }

      OPLEMU_Generate( out, count, channels );
      out    += count * channels;
      frames -= count;
      }
   }


/*
================
TS_ScheduleTask

Prepare the single MIDI task for later activation by TS_Dispatch.
Return NULL when the current task is already active.
================
*/
task *TS_ScheduleTask
   (
   void ( *Function )( task * ),
   int rate,
   int priority,
   void *data
   )

   {
   if ( OPLEMU_Task.active )
      {
      return( NULL );
      }

   OPLEMU_Task.TaskService = Function;
   OPLEMU_Task.data        = data;
   OPLEMU_Task.rate        = rate;
   OPLEMU_Task.count       = 0;
   OPLEMU_Task.priority    = priority;
   OPLEMU_Task.active      = FALSE;

   OPLEMU_FramesToTick  = 0;
   OPLEMU_TickRemainder = 0;

   return( &OPLEMU_Task );
   }


/*
================
TS_Terminate

Deactivate the known MIDI task and remove its service callback.
================
*/
int TS_Terminate
   (
   task *NodeToRemove
   )

   {
   if ( NodeToRemove != &OPLEMU_Task )
      {
      return( TASK_Warning );
      }

   OPLEMU_Task.active      = FALSE;
   OPLEMU_Task.TaskService = NULL;
   return( TASK_Ok );
   }


/*
================
TS_Dispatch

Activate the prepared MIDI task when a service callback has been installed.
================
*/
void TS_Dispatch
   (
   void
   )

   {
   if ( OPLEMU_Task.TaskService != NULL )
      {
      OPLEMU_Task.active = TRUE;
      }
   }


/*
================
TS_SetTaskRate

Change the task's tick frequency used by subsequent sample scheduling.
================
*/
void TS_SetTaskRate
   (
   task *Task,
   int rate
   )

   {
   Task->rate = rate;
   }

/*
 * MV_Shutdown stops playback with "interrupts" disabled. Closing the SDL
 *  device waits for the audio callback, which wants that same lock, so DSL
 *  has to let the callback give up. This holds the lock until the callback
 *  is stuck waiting for it, then stops playback, and fails (by timing out)
 *  if that never returns. It also checks the music callback is called and
 *  its samples reach the output alongside the digitized sound.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "SDL.h"
#include "audiolib/dsl.h"
#include "audiolib/interrup.h"

#define DIVISIONS 4
#define DIVISION  512

volatile int MV_MixPage = 0;  /* multivoc.c's, which this stands in for */

static char buffer[DIVISIONS * DIVISION];
static SDL_atomic_t services;
static SDL_atomic_t music_calls;

/* MV_ServiceVoc's part: move on to the next division. */
static void service(void)
{
    MV_MixPage = (MV_MixPage + 1) % DIVISIONS;
    SDL_AtomicIncRef(&services);
}

static void music(unsigned char *stream, int len)
{
    memset(stream, 0, len);
    SDL_AtomicIncRef(&music_calls);
}

int main(int argc, char **argv)
{
    uint32_t flags;
    unsigned rate;
    int bits, channels;
    int i;

    (void)argc; (void)argv;

    if (DSL_Init() != DSL_Ok)
    {
        printf("FAIL: DSL_Init: %s\n", SDL_GetError());
        return 1;
    }

    for (i = 0; i < (int)sizeof(buffer); i++)
        buffer[i] = (char)0x80;     /* unsigned 8-bit silence */

    if (DSL_BeginBufferedPlayback(buffer, sizeof(buffer), DIVISIONS, 22050,
            MONO_8BIT, service) != DSL_Ok)
    {
        printf("FAIL: DSL_BeginBufferedPlayback: %s\n", SDL_GetError());
        return 1;
    }

    if (DSL_GetPlaybackFormat(&rate, &bits, &channels) != DSL_Ok ||
        rate != 22050 || bits != 8 || channels != 1)
    {
        printf("FAIL: device format isn't the one asked for\n");
        return 1;
    }

    DSL_SetMusicCallback(music);

    /* Let the callback run for a while. */
    for (i = 0; i < 200 && SDL_AtomicGet(&music_calls) < 3; i++)
        SDL_Delay(10);
    if (SDL_AtomicGet(&music_calls) < 3 || SDL_AtomicGet(&services) < 3)
    {
        printf("FAIL: the audio callback isn't running (music %d, sound %d)\n",
            SDL_AtomicGet(&music_calls), SDL_AtomicGet(&services));
        return 1;
    }

    /* Hold the lock past a callback or two, then stop with it held. */
    flags = DisableInterrupts();
    SDL_Delay(200);
    DSL_StopPlayback();
    RestoreInterrupts(flags);

    DSL_Shutdown();
    printf("ok\n");
    return 0;
}

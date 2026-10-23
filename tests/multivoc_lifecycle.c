/* Mixer buffers must survive repeated initialization, playback and shutdown. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "SDL.h"
#include "audiolib/multivoc.h"
#include "audiolib/sndcards.h"
#include "audiolib/usrhooks.h"

static SDL_atomic_t finished;

int USRHOOKS_GetMem(void **ptr, uint32_t size)
{
    *ptr = malloc(size);
    return *ptr ? USRHOOKS_Ok : USRHOOKS_Error;
}

int USRHOOKS_FreeMem(void *ptr)
{
    free(ptr);
    return USRHOOKS_Ok;
}

static void completed(uint32_t value)
{
    if (value == 42) SDL_AtomicSet(&finished, 1);
}

int main(void)
{
    static char sample[2048];
    int cycle, handle, wait;

    memset(sample, 160, sizeof(sample));
    for (cycle = 0; cycle < 3; cycle++)
    {
        /* Each later init also shuts down the preceding initialized mixer. */
        if (MV_Init(SoundBlaster, 22050, 4, cycle & 1 ? 2 : 1,
                    cycle & 1 ? 16 : 8) != MV_Ok)
        {
            printf("FAIL: mixer init: %s\n", MV_ErrorString(MV_Error));
            return 1;
        }
        SDL_AtomicSet(&finished, 0);
        MV_SetCallBack(completed);
        handle = MV_PlayRaw(sample, sizeof(sample), 22050, 0,
                            255, 255, 255, 0, 42);
        for (wait = 0; wait < 200 && !SDL_AtomicGet(&finished); wait++)
            SDL_Delay(10);
        if (handle < 1 || !SDL_AtomicGet(&finished) || MV_VoicePlaying(handle))
        {
            printf("FAIL: playback did not finish in cycle %d\n", cycle);
            return 1;
        }
    }
    if (MV_Shutdown() != MV_Ok || MV_Shutdown() != MV_Ok) return 1;
    puts("multivoc_lifecycle: ok");
    return 0;
}

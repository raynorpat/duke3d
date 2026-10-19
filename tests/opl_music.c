/*
 * Plays a one-note MIDI song through MIDI, AL_MIDI and the emulated OPL3,
 *  the way the game does but without an audio device, and checks that the
 *  note sounds and that it ends when the song's tempo says it should. The
 *  song sets 60 BPM, half MIDI's default, so a player that missed the
 *  tempo change would end the note half a second early. It then plays it
 *  again at half the music volume, which must come out at about half the
 *  level, as the game's music volume does.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "audiolib/music.h"
#include "audiolib/midi.h"
#include "audiolib/al_midi.h"
#include "audiolib/oplemu.h"
#include "audiolib/sndcards.h"
#include "audiolib/usrhooks.h"

#define RATE 44100
#define WINDOW ( RATE / 20 )
#define WINDOWS 40

/* The game supplies these to the audio library. */
int MUSIC_SoundDevice = SoundBlaster;

int USRHOOKS_GetMem(void **ptr, uint32_t size)
{
    *ptr = malloc(size);
    return (*ptr == NULL) ? USRHOOKS_Error : USRHOOKS_Ok;
}

int USRHOOKS_FreeMem(void *ptr)
{
    free(ptr);
    return USRHOOKS_Ok;
}

static unsigned char song[] =
{
    'M', 'T', 'h', 'd', 0, 0, 0, 6,
    0, 0,                               /* format 0 */
    0, 1,                               /* one track */
    0, 96,                              /* 96 ticks per beat */
    'M', 'T', 'r', 'k', 0, 0, 0, 23,
    0x00, 0xff, 0x51, 0x03, 0x0f, 0x42, 0x40,   /* 1,000,000 us per beat */
    0x00, 0xc0, 0x00,                   /* piano */
    0x00, 0x90, 0x3c, 0x64,             /* middle C on */
    0x60, 0x80, 0x3c, 0x40,             /* off one beat later */
    0x00, 0xff, 0x2f, 0x00
};

static midifuncs funcs;

/* Plays the song, filling in the RMS level of each 50 ms window. */
static int play(double *level)
{
    static int16_t buf[WINDOW * 2];
    int w, i;

    if (MIDI_PlaySong(song, MUSIC_PlayOnce) != MIDI_Ok)
    {
        printf("FAIL: MIDI_PlaySong refused the song\n");
        return 0;
    }

    for (w = 0; w < WINDOWS; w++)
    {
        double sum = 0;

        OPLEMU_Render(buf, WINDOW, 2);
        for (i = 0; i < WINDOW * 2; i++)
            sum += (double)buf[i] * buf[i];
        level[w] = sqrt(sum / (WINDOW * 2));
        printf("%4d ms  %8.1f\n", w * 50, level[w]);
    }

    MIDI_StopSong();
    return 1;
}

int main(void)
{
    double level[WINDOWS];
    double half[WINDOWS];
    int failures = 0;
    int w, i;

    OPLEMU_Init(RATE);
    AL_Init(SoundBlaster);

    funcs.NoteOff       = AL_NoteOff;
    funcs.NoteOn        = AL_NoteOn;
    funcs.ControlChange = AL_ControlChange;
    funcs.ProgramChange = AL_ProgramChange;
    funcs.PitchBend     = AL_SetPitchBend;
    funcs.SetVolume     = OPLEMU_SetVolume;
    funcs.GetVolume     = OPLEMU_GetVolume;
    MIDI_SetMidiFuncs(&funcs);

    for (i = 0; i < 128; i++)
        MIDI_PatchMap[i] = i;

    MIDI_SetVolume(255);
    if (!play(level))
        return 1;

    /* Sounding from the start, and only decaying slowly (the piano's own
     * decay) until the note ends at 1 s... */
    if (level[0] < 100)
    {
        printf("FAIL: the note doesn't sound\n");
        failures++;
    }
    for (w = 1; w < 20; w++)
    {
        if (level[w] < level[w - 1] * 0.8)
        {
            printf("FAIL: released early, at %d ms\n", w * 50);
            failures++;
        }
    }

    /* ...then released, and silent half a second later. */
    if (level[21] > level[19] / 2)
    {
        printf("FAIL: not released at 1 s\n");
        failures++;
    }
    for (w = 30; w < WINDOWS; w++)
    {
        if (level[w] > 1)
        {
            printf("FAIL: still sounding at %d ms\n", w * 50);
            failures++;
        }
    }

    /* Half the volume, half the level: not the far quieter result of
     * turning down every MIDI channel's own volume. */
    MIDI_SetVolume(128);
    if (!play(half))
        return 1;
    for (w = 0; w < 20; w++)
    {
        double ratio = half[w] / level[w];

        if (ratio < 0.45 || ratio > 0.55)
        {
            printf("FAIL: half volume gives %.2f of the level at %d ms\n", ratio, w * 50);
            failures++;
        }
    }

    if (failures)
        return 1;
    printf("ok\n");
    return 0;
}

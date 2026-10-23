/*
 * Jim Dose's MUSIC_* routines over his MIDI and AL_MIDI code and an
 *   emulated OPL3, played through DSL (SDL audio). This takes the place of
 *   his MUSIC.C, which drove the DOS sound cards.
 *
 * Based on Ryan C. Gordon's SDL_mixer version. (icculus@clutteredmind.org)
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "music.h"
#include "midi.h"
#include "al_midi.h"
#include "oplemu.h"
#include "dsl.h"
#include "interrup.h"
#include "util.h"

#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )

#define DUKESND_DEBUG       "DUKESND_DEBUG"

int MUSIC_ErrorCode = MUSIC_Ok;

static char warningMessage[80];
static char errorMessage[80];
static FILE *debug_file = NULL;
static int initialized_debugging = 0;

// This gets called all over the place for information and debugging messages.
//  If the user set the DUKESND_DEBUG environment variable, the messages
//  go to the file that is specified in that variable. Otherwise, they
//  are ignored for the expense of the function call. If DUKESND_DEBUG is
//  set to "-" (without the quotes), then the output goes to stdout.
static void musdebug(const char *fmt, ...)
{
    va_list ap;

    if (debug_file)
    {
        fprintf(debug_file, "DUKEMUS: ");
        va_start(ap, fmt);
        vfprintf(debug_file, fmt, ap);
        va_end(ap);
        fprintf(debug_file, "\n");
        fflush(debug_file);
    } // if
} // musdebug

static void init_debugging(void)
{
    const char *envr;

    if (initialized_debugging)
        return;

    envr = getenv(DUKESND_DEBUG);
    if (envr != NULL)
    {
        if (strcmp(envr, "-") == 0)
            debug_file = stdout;
        else
            debug_file = fopen(envr, "w");

        if (debug_file == NULL)
            fprintf(stderr, "DUKESND: -WARNING- Could not open debug file!\n");
        else
            setbuf(debug_file, NULL);
    } // if

    initialized_debugging = 1;
} // init_debugging

static void setWarningMessage(const char *msg)
{
    strncpy(warningMessage, msg, sizeof (warningMessage));
    // strncpy() doesn't add the null char if there isn't room...
    warningMessage[sizeof (warningMessage) - 1] = '\0';
    musdebug("Warning message set to [%s].", warningMessage);
} // setErrorMessage


static void setErrorMessage(const char *msg)
{
    strncpy(errorMessage, msg, sizeof (errorMessage));
    // strncpy() doesn't add the null char if there isn't room...
    errorMessage[sizeof (errorMessage) - 1] = '\0';
    musdebug("Error message set to [%s].", errorMessage);
} // setErrorMessage

// The music functions...

char *MUSIC_ErrorString(int ErrorNumber)
{
    switch (ErrorNumber)
    {
        case MUSIC_Warning:
            return(warningMessage);

        case MUSIC_Error:
            return(errorMessage);

        case MUSIC_Ok:
            return("OK; no error.");

        case MUSIC_ASSVersion:
            return("Incorrect sound library version.");

        case MUSIC_SoundCardError:
            return("General sound card error.");

        case MUSIC_InvalidCard:
            return("Invalid sound card.");

        case MUSIC_MidiError:
            return("MIDI error.");

        case MUSIC_MPU401Error:
            return("MPU401 error.");

        case MUSIC_TaskManError:
            return("Task Manager error.");

        case MUSIC_FMNotDetected:
            return("FM not detected error.");


        default:
            return("Unknown error.");
    } // switch
} // MUSIC_ErrorString




// The music is Jim Dose's MIDI and AL_MIDI (FM) code, as DOS played it on
// a Sound Blaster 16, writing to an emulated OPL3 that DSL pulls samples
// from on the audio thread. Every call into MIDI and AL_MIDI disables
// "interrupts", which holds the audio callback off.

// MIDI picks each song's EMIDI arrangement for this card.
int MUSIC_SoundDevice = -1;

static int music_initialized = 0;
static int music_hooked = 0;
static int music_bits = 16;
static int music_channels = 2;
static midifuncs music_midifuncs;

/*
================
music_mixer

Render OPL output in bounded signed-16-bit blocks and store it in the
SDL device's channel count and 8-bit or 16-bit sample representation.
================
*/
static void music_mixer(unsigned char *stream, int len)
{
    int16_t buf[1024 * 2];
    int frames, count, i, c;
    int bytes = music_bits / 8;

    /* The SDL request is in bytes; OPLEMU_Render consumes frames with
     * one sample per channel. */
    frames = len / (bytes * music_channels);

    while (frames > 0)
    {
        count = min(frames, 1024);
        OPLEMU_Render(buf, count, (music_channels == 1) ? 1 : 2);

        for (i = 0; i < count; i++)
        {
            for (c = 0; c < music_channels; c++)
            {
                int16_t s = 0;
                if (music_channels == 1)
                    s = buf[i];
                else if (c < 2)
                    s = buf[i * 2 + c];

                if (bytes == 1)
                    *stream++ = (unsigned char) ((s >> 8) + 128);
                else
                {
                    *(int16_t *) stream = s;
                    stream += 2;
                }
            }
        }

        frames -= count;
    }
} // music_mixer


/*
================
MUSIC_Init

Initialize MIDI/OPL against the mixer format when playback is available
and install the music producer on that effects device.
================
*/
int MUSIC_Init(int SoundCard, int Address)
{
    int i;
    unsigned freq = 44100;
    int bits = 16;
    int channels = 2;

    init_debugging();

    musdebug("INIT! card=>%d, address=>%d...", SoundCard, Address);

    if (music_initialized)
    {
        setErrorMessage("Music system is already initialized.");
        return(MUSIC_Error);
    } // if
    
    if (SoundCard != SoundScape) // We pretend there's a SoundScape installed.
    {
        setErrorMessage("Card not found.");
        musdebug("We pretend to be an Ensoniq SoundScape only.");
        return(MUSIC_Error);
    } // if

    // Without sound effects there is no mixer to play through; the song
    //  is still sequenced, just never heard.
    music_hooked = (DSL_GetPlaybackFormat(&freq, &bits, &channels) == DSL_Ok);
    music_bits = bits;
    music_channels = channels;

    for (i = 0; i < 128; i++)
        MIDI_PatchMap[i] = i;

    MUSIC_SoundDevice = SoundBlaster;
    OPLEMU_Init(freq);
    AL_Init(MUSIC_SoundDevice);

    music_midifuncs.NoteOff           = AL_NoteOff;
    music_midifuncs.NoteOn            = AL_NoteOn;
    music_midifuncs.PolyAftertouch    = NULL;
    music_midifuncs.ControlChange     = AL_ControlChange;
    music_midifuncs.ProgramChange     = AL_ProgramChange;
    music_midifuncs.ChannelAftertouch = NULL;
    music_midifuncs.PitchBend         = AL_SetPitchBend;
    music_midifuncs.ReleasePatches    = NULL;
    music_midifuncs.LoadPatch         = NULL;
    // As on a Sound Blaster with a mixer: the music volume is the card's
    //  FM volume, and the notes themselves play at full level.
    music_midifuncs.SetVolume         = OPLEMU_SetVolume;
    music_midifuncs.GetVolume         = OPLEMU_GetVolume;
    MIDI_SetMidiFuncs(&music_midifuncs);

    if (music_hooked)
        DSL_SetMusicCallback(music_mixer);

    music_initialized = 1;
    return(MUSIC_Ok);
} // MUSIC_Init


/*
================
MUSIC_Shutdown

Stop sequencing, detach the music callback and reset the emulated voices.
================
*/
int MUSIC_Shutdown(void)
{
    musdebug("shutting down sound subsystem.");

    if (!music_initialized)
    {
        setErrorMessage("Music system is not currently initialized.");
        return(MUSIC_Error);
    } // if

    MUSIC_StopSong();

    if (music_hooked)
        /* Detach under the audio lock before tearing down emulated state
         * that the callback would otherwise still access. */
        DSL_SetMusicCallback(NULL);
    music_hooked = 0;

    AL_Shutdown();

    MUSIC_SoundDevice = -1;
    music_initialized = 0;

    return(MUSIC_Ok);
} // MUSIC_Shutdown


void MUSIC_SetMaxFMMidiChannel(int channel)
{
    uint32_t flags;

    flags = DisableInterrupts();
    AL_SetMaxMidiChannel(channel);
    RestoreInterrupts(flags);
} // MUSIC_SetMaxFMMidiChannel


void MUSIC_SetVolume(int volume)
{
    uint32_t flags;

    volume = max(0, volume);
    volume = min(volume, 255);

    flags = DisableInterrupts();
    MIDI_SetVolume(volume);
    RestoreInterrupts(flags);
} // MUSIC_SetVolume


void MUSIC_SetMidiChannelVolume(int channel, int volume)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_SetUserChannelVolume(channel, volume);
    RestoreInterrupts(flags);
} // MUSIC_SetMidiChannelVolume


void MUSIC_ResetMidiChannelVolumes(void)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_ResetUserChannelVolume();
    RestoreInterrupts(flags);
} // MUSIC_ResetMidiChannelVolumes


int MUSIC_GetVolume(void)
{
    return(MIDI_GetVolume());
} // MUSIC_GetVolume


void MUSIC_SetLoopFlag(int loopflag)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_SetLoopFlag(loopflag);
    RestoreInterrupts(flags);
} // MUSIC_SetLoopFlag


int MUSIC_SongPlaying(void)
{
    return(MIDI_SongPlaying());
} // MUSIC_SongPlaying


void MUSIC_Continue(void)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_ContinueSong();
    RestoreInterrupts(flags);
} // MUSIC_Continue


void MUSIC_Pause(void)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_PauseSong();
    RestoreInterrupts(flags);
} // MUSIC_Pause


int MUSIC_StopSong(void)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_StopSong();
    RestoreInterrupts(flags);

    return(MUSIC_Ok);
} // MUSIC_StopSong


/*
================
MUSIC_PlaySong

Replace the previous MIDI song under the shared audio lock.
The caller retains the song bytes used by the track cursors.
================
*/
int MUSIC_PlaySong(unsigned char *song, int loopflag)
{
    uint32_t flags;
    int status;

    MUSIC_StopSong();

    flags = DisableInterrupts();
    status = MIDI_PlaySong(song, loopflag);
    RestoreInterrupts(flags);

    if (status != MIDI_Ok)
    {
        setErrorMessage("Error playing MIDI file.");
        return(MUSIC_Warning);
    } // if

    return(MUSIC_Ok);
} // MUSIC_PlaySong


void MUSIC_SetContext(int context)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_SetContext(context);
    RestoreInterrupts(flags);
} // MUSIC_SetContext


int MUSIC_GetContext(void)
{
    return(MIDI_GetContext());
} // MUSIC_GetContext


void MUSIC_SetSongTick(uint32_t PositionInTicks)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_SetSongTick(PositionInTicks);
    RestoreInterrupts(flags);
} // MUSIC_SetSongTick


void MUSIC_SetSongTime(uint32_t milliseconds)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_SetSongTime(milliseconds);
    RestoreInterrupts(flags);
}// MUSIC_SetSongTime


void MUSIC_SetSongPosition(int measure, int beat, int tick)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_SetSongPosition(measure, beat, tick);
    RestoreInterrupts(flags);
} // MUSIC_SetSongPosition


void MUSIC_GetSongPosition(songposition *pos)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_GetSongPosition(pos);
    RestoreInterrupts(flags);
} // MUSIC_GetSongPosition


void MUSIC_GetSongLength(songposition *pos)
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_GetSongLength(pos);
    RestoreInterrupts(flags);
} // MUSIC_GetSongLength


// MUSIC.C fades with a second TASK_MAN task, which OPLEMU doesn't run.
//  Nothing in Duke fades, so this just jumps to the new volume.
int MUSIC_FadeVolume(int tovolume, int milliseconds)
{
    MUSIC_SetVolume(tovolume);
    return(MUSIC_Ok);
} // MUSIC_FadeVolume


int MUSIC_FadeActive(void)
{
    return(FALSE);
} // MUSIC_FadeActive


void MUSIC_StopFade(void)
{
} // MUSIC_StopFade


void MUSIC_RerouteMidiChannel(int channel, int (*function)( int event, int c1, int c2 ))
{
    uint32_t flags;

    flags = DisableInterrupts();
    MIDI_RerouteMidiChannel(channel, function);
    RestoreInterrupts(flags);
} // MUSIC_RerouteMidiChannel


// Duke's own FM instruments, D3DTIMBR.TMB, loaded by loadtmb().
void MUSIC_RegisterTimbreBank(unsigned char *timbres)
{
    uint32_t flags;

    flags = DisableInterrupts();
    AL_RegisterTimbreBank(timbres);
    RestoreInterrupts(flags);
} // MUSIC_RegisterTimbreBank


// end of fx_man.c ...

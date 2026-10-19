#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dsl.h"
#include "util.h"
#include "interrup.h"

#include "SDL.h"

extern volatile int MV_MixPage;

static int DSL_ErrorCode = DSL_Ok;

static int mixer_initialized;

static void ( *_CallBackFunc )( void );
static volatile char *_BufferStart;
static int _BufferSize;
static int _NumDivisions;
static int _SampleRate;
static int _remainder;

static SDL_AudioDeviceID audio_device;
static SDL_AudioSpec audio_spec;
static void ( *_MusicFunc )( unsigned char *stream, int len );

/* DisableInterrupts() takes this, and the audio callback holds it while it
   mixes, as the sound card interrupt couldn't run while interrupts were off.
   It isn't the device's own lock: MV_Shutdown closes the device with it
   held, and closing waits for the callback, so the callback must be able to
   give up on it then (audio_closing). */
static SDL_mutex *audio_lock;
static SDL_atomic_t audio_closing;

/*
possible todo ideas: cache sdl error messages.
*/

char *DSL_ErrorString( int ErrorNumber )
{
	char *ErrorString;
	
	switch (ErrorNumber) {
		case DSL_Warning:
		case DSL_Error:
			ErrorString = DSL_ErrorString(DSL_ErrorCode);
			break;
		
		case DSL_Ok:
			ErrorString = "SDL Driver ok.";
			break;
		
		case DSL_SDLInitFailure:
			ErrorString = "SDL Audio initialization failed.";
			break;
		
		case DSL_MixerActive:
			ErrorString = "SDL Mixer already initialized.";
			break;	
	
		case DSL_MixerInitFailure:
			ErrorString = "SDL Mixer initialization failed.";
			break;
			
		default:
			ErrorString = "Unknown SDL Driver error.";
			break;
	}
	
	return ErrorString;
}

static void DSL_SetErrorCode(int ErrorCode)
{
	DSL_ErrorCode = ErrorCode;
}

int DSL_Init( void )
{
	DSL_SetErrorCode(DSL_Ok);
	
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		DSL_SetErrorCode(DSL_SDLInitFailure);
		
		return DSL_Error;
	}

	/* Kept for good: a sound restart mustn't swap it under a holder. */
	if (audio_lock == NULL) {
		audio_lock = SDL_CreateMutex();
	}
	
	return DSL_Ok;
}

void DSL_Shutdown( void )
{
	DSL_StopPlayback();
}

static void mixer_callback(void *udata, Uint8 *stream, int len)
{
	Uint8 *fxptr;
	int copysize;

	while (SDL_TryLockMutex(audio_lock) != 0) {
		if (SDL_AtomicGet(&audio_closing)) {
			SDL_memset(stream, audio_spec.silence, len);
			return;
		}
		SDL_Delay(1);
	}

	SDL_memset(stream, audio_spec.silence, len);

	if (_MusicFunc) {
		_MusicFunc(stream, len);
	}

	/* The digitized sound comes from MV_ServiceVoc a division at a time;
	   the rest of a division carries over to the next callback. */
	while (len > 0) {
		if (_remainder == 0) {
			_CallBackFunc();
			_remainder = _BufferSize;
		}

		fxptr = (Uint8 *)(&_BufferStart[MV_MixPage * _BufferSize]);
		copysize = min(len, _remainder);

		SDL_MixAudioFormat(stream, fxptr + (_BufferSize - _remainder),
			audio_spec.format, copysize, SDL_MIX_MAXVOLUME);

		len -= copysize;
		_remainder -= copysize;
		stream += copysize;
	}

	SDL_UnlockMutex(audio_lock);
}

int   DSL_BeginBufferedPlayback( char *BufferStart,
      int BufferSize, int NumDivisions, unsigned SampleRate,
      int MixMode, void ( *CallBackFunc )( void ) )
{
	SDL_AudioSpec want;
	int chunksize;
		
	if (mixer_initialized) {
		DSL_SetErrorCode(DSL_MixerActive);
		
		return DSL_Error;
	}
	
	_CallBackFunc = CallBackFunc;
	_BufferStart = BufferStart;
	_BufferSize = (BufferSize / NumDivisions);
	_NumDivisions = NumDivisions;
	_SampleRate = SampleRate;

	_remainder = 0;

/*
	23ms is typically ideal (11025,22050,44100)
	46ms isn't bad
*/
	
	chunksize = 512;
	
	if (SampleRate >= 16000) chunksize *= 2;
	if (SampleRate >= 32000) chunksize *= 2;

	SDL_zero(want);
	want.freq = SampleRate;
	want.format = (MixMode & SIXTEEN_BIT) ? AUDIO_S16SYS : AUDIO_U8;
	want.channels = (MixMode & STEREO) ? 2 : 1;
	want.samples = chunksize;
	want.callback = mixer_callback;

	/* No changes allowed: MV_ServiceVoc mixes in exactly this format, and
	   SDL converts it for the hardware. */
	audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &audio_spec, 0);
	if (audio_device == 0) {
		DSL_SetErrorCode(DSL_MixerInitFailure);
		
		return DSL_Error;
	}

	SDL_PauseAudioDevice(audio_device, 0);
	
	mixer_initialized = 1;
	
	return DSL_Ok;
}

void DSL_StopPlayback( void )
{
	if (mixer_initialized) {
		SDL_AtomicSet(&audio_closing, 1);
		SDL_CloseAudioDevice(audio_device);
		SDL_AtomicSet(&audio_closing, 0);
		audio_device = 0;
	}
	
	mixer_initialized = 0;
}

unsigned DSL_GetPlaybackRate( void )
{
	return _SampleRate;
}

/* The music driver's way in: the device's format, and a function that
   writes music into each buffer before the digitized sound is mixed on. */
int DSL_GetPlaybackFormat( unsigned *rate, int *bits, int *channels )
{
	if (!mixer_initialized) {
		return DSL_Error;
	}

	*rate = audio_spec.freq;
	*bits = SDL_AUDIO_BITSIZE(audio_spec.format);
	*channels = audio_spec.channels;
	return DSL_Ok;
}

void DSL_SetMusicCallback( void ( *func )( unsigned char *stream, int len ) )
{
	uint32_t flags;

	flags = DisableInterrupts();
	_MusicFunc = func;
	RestoreInterrupts(flags);
}

uint32_t DisableInterrupts( void )
{
	if (audio_lock) {
		SDL_LockMutex(audio_lock);
	}
	return 0;
}

void RestoreInterrupts( uint32_t flags )
{
	if (audio_lock) {
		SDL_UnlockMutex(audio_lock);
	}
}


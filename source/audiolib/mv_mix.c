/* Kernel names describe output depth; a trailing 16 selects 16-bit input.
 * Position/rate are 16.16 source-sample units, length is output frames, and
 * MV_SampleSize is the byte stride between output frames. */
#include <stdint.h>
#include "multivoc.h"

extern char  *MV_MixDestination;
extern uint32_t MV_MixPosition;

extern char *MV_LeftVolume;
extern char *MV_RightVolume;

extern unsigned char *MV_HarshClipTable;

extern int MV_RightChannelOffset;
extern int MV_SampleSize;

/*
================
MV_Mix8BitMono

Resample an 8-bit mono source into the 8-bit output channel using
volume and saturation lookups, then publish the advanced cursors.
================
*/
void MV_Mix8BitMono( uint32_t position, uint32_t rate,
   const char *start, uint32_t length )
{
	const unsigned char *src;
	unsigned char *dest;
	unsigned int i;

	src = (const unsigned char *)start;
	dest = (unsigned char *)MV_MixDestination;

	for (i = 0; i < length; i++) {
		int s = src[position >> 16];
		int d = *dest;
		
		/* Volume entries are 16-bit slots; this 8-bit output path reads the
		 * byte contribution used by its unsigned-sample clipping table. */
		s = MV_LeftVolume[s * 2];

		s += d;
		
		s = MV_HarshClipTable[s + 0x80];
		
		*dest = (s & 0xff);
		
		position += rate;
		dest += MV_SampleSize;
	}
	
	MV_MixPosition = position;
	MV_MixDestination = (char *)dest;
}

/*
================
MV_Mix8BitStereo

Resample an 8-bit mono source into two independently scaled 8-bit channels.
================
*/
void MV_Mix8BitStereo( uint32_t position,
   uint32_t rate, const char *start, uint32_t length )
{
	const unsigned char *src;
	unsigned char *dest;
	unsigned int i;
	
	src = (const unsigned char *)start;
	dest = (unsigned char *)MV_MixDestination;

	for (i = 0; i < length; i++) {
		int s = src[(position >> 16)];
		int dl = dest[0];
		int dr = dest[MV_RightChannelOffset];
		
		dl += MV_LeftVolume[s * 2];
		dr += MV_RightVolume[s * 2];
		
		dl = MV_HarshClipTable[dl + 0x80];
		dr = MV_HarshClipTable[dr + 0x80];
		
		dest[0] = (dl & 0xff);
		dest[MV_RightChannelOffset] = (dr & 0xff);
		
		position += rate;
		dest += MV_SampleSize;
	}
	
	MV_MixPosition = position;
	MV_MixDestination = (char *)dest;
}

/*
================
MV_Mix16BitMono

Mix an 8-bit mono source into signed 16-bit output with saturation.
================
*/
void MV_Mix16BitMono( uint32_t position,
   uint32_t rate, const char *start, uint32_t length )
{
	const short *MV_LeftVolumeS;
	const unsigned char *src;
	short *dest;
	unsigned int i;

	src = (const unsigned char *)start;
	dest = (short *)MV_MixDestination;
	
	MV_LeftVolumeS = (const short *)MV_LeftVolume;
	
	for (i = 0; i < length; i++) {
		int s = src[position >> 16];
		int d = dest[0];
		
		s = MV_LeftVolumeS[s];
		
		s += d;
		
		if (s < -32768) s = -32768;
		if (s >  32767) s =  32767;
		
		*dest = (short) s;
		
		position += rate;
		dest += MV_SampleSize/2;
	}
	
	MV_MixPosition = position;
	MV_MixDestination = (char *)dest;
}

/*
================
MV_Mix16BitStereo

Mix an 8-bit mono source into independently scaled signed 16-bit channels.
================
*/
void MV_Mix16BitStereo( uint32_t position,
   uint32_t rate, const char *start, uint32_t length )
{
	const short *MV_LeftVolumeS;
	const short *MV_RightVolumeS;
	const unsigned char *src;
	short *dest;
	unsigned int i;

	src = (unsigned char *)start;
	dest = (short *)MV_MixDestination;
	
	MV_LeftVolumeS = (const short *)MV_LeftVolume;
	MV_RightVolumeS = (const short *)MV_RightVolume;
	
	for (i = 0; i < length; i++) {
		int s = src[position >> 16];
		int dl = dest[0];
		int dr = dest[MV_RightChannelOffset/2];
		
		dl += MV_LeftVolumeS[s];
		dr += MV_RightVolumeS[s];
		
		if (dl < -32768) dl = -32768;
		if (dl >  32767) dl =  32767;
		if (dr < -32768) dr = -32768;
		if (dr >  32767) dr =  32767;
		
		dest[0] = (short) dl;
		dest[MV_RightChannelOffset/2] = (short) dr;
		
		position += rate;
		/* The shared stride is in bytes; this destination is a short pointer. */
		dest += MV_SampleSize/2;
	}
	
	MV_MixPosition = position;
	MV_MixDestination = (char *)dest;
}

/*
================
MV_Mix8BitMono16

Use the high byte of each 16-bit source sample for the 8-bit mono mix.
================
*/
void MV_Mix8BitMono16( uint32_t position, uint32_t rate,
   const char *start, uint32_t length )
{
	const char *src;
	unsigned char *dest;
	unsigned int i;

	src = (const char *)start + 1;
	dest = (unsigned char *)MV_MixDestination;

	for (i = 0; i < length; i++) {
		int s = (int)src[(position >> 16) * 2] + 0x80;
		int d = *dest;
		
		s = MV_LeftVolume[s * 2];
		
		s += d;
		
		s = MV_HarshClipTable[s + 0x80];
		
		*dest = (s & 0xff);
		
		position += rate;
		dest += MV_SampleSize;
	}
	
	MV_MixPosition = position;
	MV_MixDestination = (char *)dest;
}

/*
================
MV_Mix8BitStereo16

Use each 16-bit source sample's high byte for the two-channel 8-bit mix.
================
*/
void MV_Mix8BitStereo16( uint32_t position,
   uint32_t rate, const char *start, uint32_t length )
{
	const char *src;
	unsigned char *dest;
	unsigned int i;
	
	src = (const char *)start + 1;
	dest = (unsigned char *)MV_MixDestination;
	
	for (i = 0; i < length; i++) {
		int s = src[(position >> 16) * 2] + 0x80;
		int dl = dest[0];
		int dr = dest[MV_RightChannelOffset];
		
		dl += MV_LeftVolume[s * 2];
		dr += MV_RightVolume[s * 2];
		
		dl = MV_HarshClipTable[dl + 0x80];
		dr = MV_HarshClipTable[dr + 0x80];
		
		dest[0] = (dl & 0xff);
		dest[MV_RightChannelOffset] = (dr & 0xff);
		
		position += rate;
		dest += MV_SampleSize;
	}
	
	MV_MixPosition = position;
	MV_MixDestination = (char *)dest;
}

/*
================
MV_Mix16BitMono16

Combine byte-volume lookups for a little-endian 16-bit source
and add it to signed 16-bit mono output with saturation.
================
*/
void MV_Mix16BitMono16( uint32_t position,
   uint32_t rate, const char *start, uint32_t length )
{
	const short *MV_LeftVolumeS;
	const unsigned char *src;
	short *dest;
	unsigned int i;
	
	src = (const unsigned char *)start;
	dest = (short *)MV_MixDestination;
	
	MV_LeftVolumeS = (const short *)MV_LeftVolume;
	
	for (i = 0; i < length; i++) {
		int sl = src[(position >> 16) * 2 + 0];
		/* Bias the signed high byte into the lookup's unsigned index range.
		 * The low-byte contribution is combined separately below. */
		int sh = src[(position >> 16) * 2 + 1] ^ 0x80;
		
		int d = *dest;
		
		sl = MV_LeftVolume[sl * 2 + 1];
		sh = MV_LeftVolumeS[sh];
		
		d = sl + sh + 0x80 + d;
		
		if (d < -32768) d = -32768;
		if (d >  32767) d =  32767;
		
		*dest = (short) d;
		
		position += rate;
		dest += MV_SampleSize/2;
	}
	
	MV_MixPosition = position;
	MV_MixDestination = (char *)dest;
}

/*
================
MV_Mix16BitStereo16

Mix a little-endian 16-bit mono source into independently scaled
signed 16-bit channels using split-byte lookup contributions.
================
*/
void MV_Mix16BitStereo16( uint32_t position,
   uint32_t rate, const char *start, uint32_t length )
{
	const short *MV_LeftVolumeS;
	const short *MV_RightVolumeS;
	const unsigned char *src;
	short *dest;
	unsigned int i;
	
	src = (const unsigned char *)start;
	dest = (short *)MV_MixDestination;
	
	MV_LeftVolumeS = (const short *)MV_LeftVolume;
	MV_RightVolumeS = (const short *)MV_RightVolume;
	
	for (i = 0; i < length; i++) {
		int sl = src[(position >> 16) * 2 + 0];
		int sh = src[(position >> 16) * 2 + 1] ^ 0x80;
		
		int dl = dest[0];
		int dr = dest[MV_RightChannelOffset/2];
		
		int sll = MV_LeftVolume[sl * 2 + 1];
		int slh = MV_LeftVolumeS[sh];
		
		int srl = MV_RightVolume[sl * 2 + 1];
		int srh = MV_RightVolumeS[sh];
		
		dl = sll + slh + 0x80 + dl;
		dr = srl + srh + 0x80 + dr;
		
		if (dl < -32768) dl = -32768;
		if (dl >  32767) dl =  32767;
		if (dr < -32768) dr = -32768;
		if (dr >  32767) dr =  32767;
		
		dest[0] = (short) dl;
		dest[MV_RightChannelOffset/2] = (short) dr;
		
		position += rate;
		dest += MV_SampleSize/2;
	}
	
	MV_MixPosition = position;
	MV_MixDestination = (char *)dest;
}

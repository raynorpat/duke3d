//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "duke3d.h"
#include "types.h"
#include "develop.h"
#include "util_lib.h"
#include "_animlib.h"
#include "animlib.h"

//****************************************************************************
//
// GLOBALS
//
//****************************************************************************

//****************************************************************************
//
// LOCALS
//
//****************************************************************************
anim_t * anim=NULL;
static boolean Anim_Started = false;

//****************************************************************************
//
//      CheckAnimStarted ()
//
// Require an active decoder before accessing the shared animation state.
//
//****************************************************************************

void CheckAnimStarted ( char * funcname )
   {
   if (!Anim_Started)
      Error("ANIMLIB_%s: Anim has not been initialized\n",funcname);
   }
//****************************************************************************
//
//      findpage ()
//              - given a frame number return the large page number it resides in
//
//****************************************************************************

uint16 findpage (uint16 framenumber)
   {
   uint16 i;

   CheckAnimStarted ( "findpage" );
   for(i=0; i<anim->lpheader.nLps; i++)
      {
      if
         (
         anim->LpArray[i].baseRecord <= framenumber &&
         anim->LpArray[i].baseRecord + anim->LpArray[i].nRecords > framenumber
         )
         return(i);
      }
   return(i);
   }


//****************************************************************************
//
//      loadpage ()
//      - seek out and load in the large page specified
//
//****************************************************************************

void loadpage (uint16 pagenumber, uint16 *pagepointer)
   {
   int32 size;
   byte * buffer;

   CheckAnimStarted ( "loadpage" );
   buffer = anim->buffer;
   /* The page number is the cache key; callers retain the same page buffer
    * between requests so a cache hit can reuse its previous contents. */
   if (anim->curlpnum != pagenumber)
      {
      anim->curlpnum = pagenumber;
      /* Large pages begin after the header/palette/index area and occupy
       * 64 KiB slots in the caller's file image. */
      buffer += 0xb00 + (pagenumber*0x10000);
      size = sizeof(lp_descriptor);
      memcpy(&anim->curlp,buffer,size);
      #if PLATFORM_BIGENDIAN
      anim->curlp.baseRecord = BUILDSWAP_INTEL16(anim->curlp.baseRecord);
      anim->curlp.nRecords = BUILDSWAP_INTEL16(anim->curlp.nRecords);
      anim->curlp.nBytes = BUILDSWAP_INTEL16(anim->curlp.nBytes);
      #endif
      buffer += size + sizeof(uint16);

      // most of pagepointer is 8-bit data, the 16-bit stuff is byteswapped
      //  on the fly in renderframe() and CPlayRunSkipDump(), below.  --ryan.
      memcpy(pagepointer,buffer,anim->curlp.nBytes+(anim->curlp.nRecords*2));
      }
   }


//****************************************************************************
//
//      CPlayRunSkipDump ()
//      - This version of the decompressor is here for portability to non PC's
//
// Apply run, skip and literal operations to the existing image.
// Unwritten pixels carry forward from earlier records.
//
//****************************************************************************

void CPlayRunSkipDump (char *srcP, char *dstP)
   {
   signed char cnt;
   uint16 wordCnt;
   byte pixel;

nextOp:
   /* A positive byte copies literals, zero repeats a colour, and a negative
    * byte skips unchanged pixels or introduces the extended operation. */
   cnt = (signed char) *srcP++;
   if (cnt > 0)
      goto dump;
   if (cnt == 0)
      goto run;
   cnt -= 0x80;
   if (cnt == 0)
      goto longOp;
/* shortSkip */
   /* Skipped pixels retain the previous image; these records are deltas,
    * not independent full-frame bitmaps. */
   dstP += cnt;                    /* adding 7-bit count to 32-bit pointer */
   goto nextOp;
dump:
   do
      {
      *dstP++ = *srcP++;
      } while (--cnt);
   goto nextOp;
run:
   wordCnt = (byte)*srcP++;                /* 8-bit unsigned count */
   pixel = *srcP++;
   do
      {
      *dstP++ = pixel;
      } while (--wordCnt);

   goto nextOp;
longOp:
   wordCnt = *((uint16 *)srcP);

#if PLATFORM_BIGENDIAN
   wordCnt = BUILDSWAP_INTEL16(wordCnt);
#endif

   srcP += sizeof(uint16);
   /* The extended word is little-endian: a positive signed value skips,
    * zero terminates, and the high bits select literal or repeated data. */
   if ((int16)wordCnt <= 0)
      goto notLongSkip;       /* Do SIGNED test. */

/* longSkip. */
   dstP += wordCnt;
   goto nextOp;

notLongSkip:
   if (wordCnt == 0)
      goto stop;
   wordCnt -= 0x8000;              /* Remove sign bit. */
   if (wordCnt >= 0x4000)
      goto longRun;

/* longDump. */
   do
      {
      *dstP++ = *srcP++;
      } while (--wordCnt);
   goto nextOp;

longRun:
   wordCnt -= 0x4000;              /* Clear "longRun" bit. */
   pixel = *srcP++;
   do
      {
      *dstP++ = pixel;
      } while (--wordCnt);
   goto nextOp;

stop:   /* all done */
   ;
   }


//****************************************************************************
//
//      renderframe ()
//      - draw the frame sepcified from the large page in the buffer pointed to
//
//****************************************************************************

void renderframe (uint16 framenumber, uint16 *pagepointer)
   {
   uint16 offset=0;
   uint16 i;
   uint16 destframe;
   uint16 pp1;
   byte *ppointer;

   CheckAnimStarted ( "renderframe" );
   destframe = framenumber - anim->curlp.baseRecord;

   for(i = 0; i < destframe; i++)
      {
      offset += BUILDSWAP_INTEL16(pagepointer[i]);
      }
   ppointer = (byte *)pagepointer;

   /* The page starts with one 16-bit length per record; sum earlier lengths
    * to reach this frame's record after that table. */
   ppointer+=anim->curlp.nRecords*2+offset;
   if(ppointer[1])
      {
      uint16 pp1 = BUILDSWAP_INTEL16(((uint16 *)ppointer)[1]);
      /* Round the optional record header data up to an even byte boundary
       * before passing the compressed pixels to the delta decoder. */
      ppointer += (4 + (pp1 + (pp1 & 1)));
      }
   else
      {
      ppointer+=4;
      }

   CPlayRunSkipDump (ppointer, anim->imagebuffer);
   }


//****************************************************************************
//
//      drawframe ()
//      - high level frame draw routine
//
//****************************************************************************

void drawframe (uint16 framenumber)
   {
   CheckAnimStarted ( "drawframe" );
   loadpage(findpage(framenumber), anim->thepage);
   renderframe(framenumber, anim->thepage);
   }


//****************************************************************************
//
//      ANIM_LoadAnim ()
//
// Attach the caller's ANM bytes and initialize header, palette and page metadata.
// The caller must provide anim storage before entering this routine.
//
//****************************************************************************

void ANIM_LoadAnim (char * buffer)
{
   uint16 i;
   int32 size;

   assert(sizeof(lp_descriptor) == 6);
   assert(sizeof(lpfileheader) == 128);
   assert(sizeof(anim->LpArray) == 1536);

   if (!Anim_Started) Anim_Started = true;

   /* playanm supplies both anim storage and the file bytes from its cache
    * allocation. This decoder borrows them and performs no allocation here. */
   anim->buffer = buffer;
   anim->curlpnum = 0xffff;
   anim->currentframe = -1;
   size = sizeof(lpfileheader);
   memcpy(&anim->lpheader, buffer, size );

#if PLATFORM_BIGENDIAN
   anim->lpheader.id = BUILDSWAP_INTEL32(anim->lpheader.id);
   anim->lpheader.maxLps = BUILDSWAP_INTEL16(anim->lpheader.maxLps);
   anim->lpheader.nLps = BUILDSWAP_INTEL16(anim->lpheader.nLps);
   anim->lpheader.nRecords = BUILDSWAP_INTEL32(anim->lpheader.nRecords);
   anim->lpheader.maxRecsPerLp = BUILDSWAP_INTEL16(anim->lpheader.maxRecsPerLp);
   anim->lpheader.lpfTableOffset = BUILDSWAP_INTEL16(anim->lpheader.lpfTableOffset);
   anim->lpheader.contentType = BUILDSWAP_INTEL32(anim->lpheader.contentType);
   anim->lpheader.width = BUILDSWAP_INTEL16(anim->lpheader.width);
   anim->lpheader.height = BUILDSWAP_INTEL16(anim->lpheader.height);
   anim->lpheader.nFrames = BUILDSWAP_INTEL32(anim->lpheader.nFrames);
   anim->lpheader.framesPerSecond = BUILDSWAP_INTEL16(anim->lpheader.framesPerSecond);
   //uint16 pad2[29]; // 58 bytes of filler to round up to 128 bytes total.
#endif

   buffer += size+128;
   // load the color palette
   for (i = 0; i < 768; i += 3)
      {
      /* The file stores four-byte BGR entries; expose packed RGB channels
       * without carrying the unused fourth byte into the palette. */
      anim->pal[i+2] = *buffer++;
      anim->pal[i+1] = *buffer++;
      anim->pal[i] = *buffer++;
      buffer++;
      }
        // read in large page descriptors
   size = sizeof(anim->LpArray);
   memcpy(&anim->LpArray,buffer,size);

#if PLATFORM_BIGENDIAN
   for (i = 0; i < sizeof (anim->LpArray) / sizeof (anim->LpArray[0]); i++)
   {
      lp_descriptor *d = &anim->LpArray[i];
      d->baseRecord = BUILDSWAP_INTEL16(d->baseRecord);
      d->nRecords = BUILDSWAP_INTEL16(d->nRecords);
      d->nBytes = BUILDSWAP_INTEL16(d->nBytes);
   }
#endif
}

//****************************************************************************
//
//      ANIM_FreeAnim ()
//
// Mark the decoder inactive; the caller's cache allocation is not freed here.
//
//****************************************************************************

void ANIM_FreeAnim ( void )
   {
   if (Anim_Started)
      {
//      SafeFree(anim);
      Anim_Started = false;
      }
   }

//****************************************************************************
//
//      ANIM_NumFrames ()
//
// Return the header's record count used by the playback API.
//
//****************************************************************************

int32 ANIM_NumFrames ( void )
   {
   CheckAnimStarted ( "NumFrames" );
   return anim->lpheader.nRecords;
   }

//****************************************************************************
//
//      ANIM_DrawFrame ()
//
// Apply delta records up to the requested frame and return the borrowed image.
// Frame 1 applies record 0; later draws overwrite the same image storage.
//
//****************************************************************************

byte * ANIM_DrawFrame (int32 framenumber)
   {
   int32 cnt;

   CheckAnimStarted ( "DrawFrame" );
   /* The public frame number is one beyond the last applied record.
    * Forward requests resume there; earlier requests replay from record zero. */
   if ((anim->currentframe != -1) && (anim->currentframe<=framenumber))
      {
      for (cnt = anim->currentframe; cnt < framenumber; cnt++)
          drawframe (cnt);
      }
   else
      {
      for (cnt = 0; cnt < framenumber; cnt++)
         drawframe (cnt);
      }
   anim->currentframe = framenumber;
   return anim->imagebuffer;
   }

//****************************************************************************
//
//      ANIM_GetPalette ()
//
// Return the packed RGB palette inside the caller-provided animation storage.
//
//****************************************************************************

byte * ANIM_GetPalette ( void )
   {
   CheckAnimStarted ( "GetPalette" );
   return anim->pal;
   }

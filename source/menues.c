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

#include <stdint.h>
#include "duke3d.h"
#include "mouse.h"
#include "savegame.h"
#include "animlib.h"
#include "glext.h"
#include "displaymode.h"
#include "videoopts.h"

/* this should be a proper prototype included from a header file */
#if PLATFORM_UNIX
extern int stricmp(const char *x, const char *y);
#endif

extern short inputloc;
extern int recfilep;
extern char vgacompatible;
short probey=0,lastprobey=0,last_menu,globalskillsound=-1;
short sh,onbar,buttonstat,deletespot;
short last_zero,last_fifty,last_threehundred = 0;
static char fileselect = 1, menunamecnt, menuname[256][17], curpath[80], menupath[80];

int ignore_escape = 0;

// CTW - REMOVED
/* Error codes */
/*
#define eTenBnNotInWindows 3801
#define eTenBnBadGameIni 3802
#define eTenBnBadTenIni 3803
#define eTenBnBrowseCancel 3804
#define eTenBnBadTenInst 3805

int  tenBnStart(void);
void tenBnSetBrowseRtn(char *(*rtn)(char *str, int len));
void tenBnSetExitRtn(void (*rtn)(void));
void tenBnSetEndRtn(void (*rtn)(void));*/
// CTW END - REMOVED

void dummyfunc(void)
{
}

void dummymess(int i,char *c)
{
}

// CTW - REMOVED
/*
void TENtext(void)
{
    long dacount,dalastcount;

    puts("\nDuke Nukem 3D has been licensed exclusively to TEN (Total");
    puts("Entertainment Network) for wide-area networked (WAN) multiplayer");
    puts("games.\n");

    puts("The multiplayer code within Duke Nukem 3D has been highly");
    puts("customized to run best on TEN, where you'll experience fast and");
    puts("stable performance, plus other special benefits.\n");

    puts("We do not authorize or recommend the use of Duke Nukem 3D with");
    puts("gaming services other than TEN.\n");

    puts("Duke Nukem 3D is protected by United States copyright law and");
    puts("international treaty.\n");

    puts("For the best online multiplayer gaming experience, please call TEN");
    puts("at 800-8040-TEN, or visit TEN's Web Site at www.ten.net.\n");

    puts("Press any key to continue.\n");

    _bios_timeofday(0,&dacount);

    while( _bios_keybrd(1) == 0 )
    {
        _bios_timeofday(0,&dalastcount);
        if( (dacount+240) < dalastcount ) break;
    }
}
*/
// CTW END - REMOVED

/*
================
cmenu

Switch menu pages and restore the cursor row remembered for major pages.
Save confirmation pages keep the current row.
================
*/
void cmenu(short cm)
{
    current_menu = cm;

    if( (cm >= 1000 && cm <= 1009) )
        return;

    if( cm == 0 )
        probey = last_zero;
    else if(cm == 50)
        probey = last_fifty;
    else if(cm >= 300 && cm < 400)
        probey = last_threehundred;
    else if(cm == 110)
        probey = 1;
    else probey = 0;
    lastprobey = -1;
}


/*
================
savetemp

Write the supplied memory block to a temporary file for later menu use.
================
*/
void savetemp(char *fn,intptr_t daptr,int32_t dasiz)
{
    int fp;

    fp = open(fn,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

    write(fp,(char *)daptr,dasiz);

    close(fp);
}

/*
================
getangplayers

Draw the other connected players as direction markers around this player.
================
*/
void getangplayers(short snum)
{
    short i,a;

    for(i=connecthead;i>=0;i=connectpoint2[i])
    {
        if(i != snum)
        {
            a = ps[snum].ang+getangle(ps[i].posx-ps[snum].posx,ps[i].posy-ps[snum].posy);
            a = a-1024;
            rotatesprite(
                (320<<15) + (((sintable[(a+512)&2047])>>7)<<15),
                (320<<15) - (((sintable[a&2047])>>8)<<15),
                klabs(sintable[((a>>1)+768)&2047]<<2),0,APLAYER,0,ps[i].palookup,0,0,0,xdim-1,ydim-1);
        }
    }
}

/*
================
loadpheader

Read a save slot's description, level settings and preview tile
without restoring the game world.
================
*/
static int loadpheader(char spot,int32 *vn,int32 *ln,int32 *psk,int32 *nump)
{

     int32_t i;
     char filename[] = "game0.sav";
     char fullname[MAX_PATH];
     int32_t fil;
#if DUKE3D_GAMEVER >= GAMEVER_11
     int32_t bv = 0;
#endif

     filename[4] = spot+'0';
     GetPathFromEnvironment(fullname, sizeof(fullname), filename);

     if ((fil = kopen4load(fullname,0)) == -1) return(-1);

     walock[MAXTILES-3] = 255;

#if DUKE3D_GAMEVER < GAMEVER_11
     // 1.0 saves have no version: they are told by their layout.
     if(sg_check10(fil) < 0)
#else
     bv = sg_read32(fil);
     if(bv != BYTEVERSION)
#endif
     {
        FTA(114,&ps[myconnectindex]);
        kclose(fil);
        return 1;
     }

#if DUKE3D_GAMEVER < GAMEVER_14
     *nump = 1;  // 1.3D saves have no player count
#else
     *nump = sg_read32(fil);
#endif

     kdfread(tempbuf,19,1,fil);
     *vn = sg_read32(fil);
     *ln = sg_read32(fil);
     *psk = sg_read32(fil);

     if (waloff[MAXTILES-3] == 0) allocache(&waloff[MAXTILES-3],160*100,&walock[MAXTILES-3]);
     /* The saved 160 by 100 preview is exposed as a transposed Build tile
      * for the menu's rotated draw. */
     tilesizx[MAXTILES-3] = 100; tilesizy[MAXTILES-3] = 160;
     kdfread((char *)waloff[MAXTILES-3],160,100,fil);

         kclose(fil);

         return(0);
}


/*
================
loadplayer

Resolve a save slot filename and load it, or queue a network load request
when the selected version supports that protocol.
================
*/
int loadplayer(signed char spot)
{
     char fn[] = "game0.sav";
     char mpfn[] = "gameA_00.sav";
     char fullname[MAX_PATH];

     video_flushpending();  // shows frames of its own

     if(spot < 0)
     {
#if DUKE3D_GAMEVER < GAMEVER_14
        // 1.3D loads locally in a network game too; it has no load request.
        spot = -spot-1;
#else
        multiflag = 1;
        multiwhat = 0;
        multipos = -spot-1;
        return -1;
#endif
     }

     if( multiflag == 2 && multiwho != myconnectindex )
     {
         mpfn[4] = spot + 'A';

         if(ud.multimode > 9)
         {
             mpfn[6] = (multiwho/10) + '0';
             mpfn[7] = (multiwho%10) + '0';
         }
         else mpfn[7] = multiwho + '0';

         GetPathFromEnvironment(fullname, sizeof(fullname), mpfn);
     }
     else
     {
        fn[4] = spot + '0';

        GetPathFromEnvironment(fullname, sizeof(fullname), fn);
     }

     return loadplayerfile(fullname,spot);
}

static int32_t clampcount(int32_t v, int32_t hi)
{
     return v < 0 ? 0 : (v > hi ? hi : v);
}

/*
================
loadplayerfile

Restore the version-specific save layout and rebuild live script, animation
and interpolation references before restarting play.
================
*/
int loadplayerfile(char *fnptr, signed char spot)
{
     short k,music_changed;
     char scriptptrs[MAXSCRIPTSIZE];
     int32_t fil, bv, i, j, x, dos = 0, animoffs[MAXANIMATES];
     int32 nump;

     if ((fil = kopen4load(fnptr,0)) == -1) return(-1);

     CONTROL_ResetInput();
     ready2send = 0;

#if DUKE3D_GAMEVER < GAMEVER_11
     // 1.0 saves have no version: they are told by their layout (rulings.md).
     // bv is where the save's script was, for its script addresses.
     bv = sg_check10(fil);
     if(bv < 0)
     {
#if DUKE3D_GAMEVER < GAMEVER_10
        printf("%s is not a DOS 0.99 save.\n",fnptr);
#else
        printf("%s is not a DOS 1.0 save.\n",fnptr);
#endif
        FTA(114,&ps[myconnectindex]);
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        return 1;
     }
#else
     bv = sg_read32(fil);
     if(bv != BYTEVERSION)
     {
        printf("%s is a version %d save, not %d.\n",fnptr,bv,BYTEVERSION);
        FTA(114,&ps[myconnectindex]);
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        return 1;
     }
     i = sg_playerblocksize(fil);
     if(i != SG_PLAYER_BLOCK0)
     {
#if DUKE3D_GAMEVER < GAMEVER_13D
        printf("%s is not a DOS 1.1 save (player block is %d bytes).\n",fnptr,i);
#elif DUKE3D_GAMEVER < GAMEVER_14
        printf("%s is not a DOS 1.3D save (player block is %d bytes).\n",fnptr,i);
#elif DUKE3D_GAMEVER < GAMEVER_15
        printf("%s is not a DOS 1.4 save (player block is %d bytes).\n",fnptr,i);
#else
        printf("%s is not a DOS 1.5 save (player block is %d bytes).\n",fnptr,i);
#endif
        FTA(114,&ps[myconnectindex]);
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        return 1;
     }
     if(!sg_checksave(fil))
     {
        printf("%s has an invalid or truncated save layout.\n",fnptr);
        FTA(114,&ps[myconnectindex]);
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        return 1;
     }
#endif

#if DUKE3D_GAMEVER < GAMEVER_14
     nump = 1;  // 1.3D saves have no player count
#else
     nump = sg_read32(fil);
#endif
     if(nump != numplayers)
     {
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        FTA(124,&ps[myconnectindex]);
        return 1;
     }

     if(numplayers > 1)
     {
         pub = NUMPAGES;
         pus = NUMPAGES;
         vscrn();
         drawbackground();
         menutext(160,100,0,0,"LOADING...");
         nextpage();
    }

     waitforeverybody();

         FX_StopAllSounds();
     clearsoundlocks();
         MUSIC_StopSong();

     if(numplayers > 1)
         kdfread(&buf,19,1,fil);
     else
         kdfread(&ud.savegame[spot][0],19,1,fil);

     music_changed = (music_select != (ud.volume_number*11) + ud.level_number);

         ud.volume_number = sg_read32(fil);
     ud.level_number = sg_read32(fil);
         ud.player_skill = sg_read32(fil);

         ud.m_level_number = ud.level_number;
         ud.m_volume_number = ud.volume_number;
         ud.m_player_skill = ud.player_skill;

                 //Fake read because lseek won't work with compression
     walock[MAXTILES-3] = 1;
     if (waloff[MAXTILES-3] == 0) allocache(&waloff[MAXTILES-3],160*100,&walock[MAXTILES-3]);
     tilesizx[MAXTILES-3] = 100; tilesizy[MAXTILES-3] = 160;
     kdfread((char *)waloff[MAXTILES-3],160,100,fil);

     numwalls = clampcount(sg_read16(fil),MAXWALLS);
#if DUKE3D_GAMEVER < GAMEVER_11 // 1.0 saves only the map's walls and sectors
     sg_readrecs(fil,wall,sizeof(walltype),sg_wall,numwalls,SG_WALL_SIZE,numwalls);
#else
     sg_readrecs(fil,wall,sizeof(walltype),sg_wall,MAXWALLS,SG_WALL_SIZE,MAXWALLS);
#endif
     numsectors = clampcount(sg_read16(fil),MAXSECTORS);
#if DUKE3D_GAMEVER < GAMEVER_11
     sg_readrecs(fil,sector,sizeof(sectortype),sg_sector,numsectors,SG_SECTOR_SIZE,numsectors);
#else
     sg_readrecs(fil,sector,sizeof(sectortype),sg_sector,MAXSECTORS,SG_SECTOR_SIZE,MAXSECTORS);
#endif
     sg_readrecs(fil,sprite,sizeof(spritetype),sg_sprite,MAXSPRITES,SG_SPRITE_SIZE,MAXSPRITES);
     sg_read16s(fil,headspritesect,2,MAXSECTORS+1);
     sg_read16s(fil,prevspritesect,2,MAXSPRITES);
     sg_read16s(fil,nextspritesect,2,MAXSPRITES);
     sg_read16s(fil,headspritestat,2,MAXSTATUS+1);
     sg_read16s(fil,prevspritestat,2,MAXSPRITES);
     sg_read16s(fil,nextspritestat,2,MAXSPRITES);
     numcyclers = clampcount(sg_read16(fil),MAXCYCLERS);
     sg_read16s(fil,&cyclers[0][0],12,MAXCYCLERS);
#if DUKE3D_GAMEVER < GAMEVER_13D
     memset(&ps[SG_DOS_MAXPLAYERS],0,sizeof(ps)-SG_DOS_MAXPLAYERS*sizeof(ps[0]));
     memset(&po[SG_DOS_MAXPLAYERS],0,sizeof(po)-SG_DOS_MAXPLAYERS*sizeof(po[0]));
#endif
     sg_readrecs(fil,ps,sizeof(struct player_struct),sg_player,SG_DOS_MAXPLAYERS,SG_PLAYER_DFSIZE,SG_PLAYER_DFCOUNT);
     for(i=0;i<MAXPLAYERS;i++) ps[i].palette = (char *) &palette[0];
     sg_readrecs(fil,po,sizeof(struct player_orig),sg_playerorig,SG_DOS_MAXPLAYERS,SG_PLAYERORIG_DFSIZE,SG_PLAYERORIG_DFCOUNT);
#if DUKE3D_GAMEVER < GAMEVER_11
     // 1.0 saves hittype[] here, raw, with DOS script addresses (F10-003):
     // they are made offsets once actorscrptr[] is in.
     clearbufbyte(hittype,sizeof(hittype),0L);
     sg_readrecs(fil,hittype,sizeof(struct weaponhit),sg_weaponhit,SG_DOS_MAXHITTYPES,SG_WEAPONHIT_SIZE,SG_DOS_MAXHITTYPES);
#endif
     numanimwalls = clampcount(sg_read16(fil),MAXANIMWALLS);
     sg_readrecs(fil,animwall,sizeof(struct animwalltype),sg_animwall,MAXANIMWALLS,SG_ANIMWALL_SIZE*MAXANIMWALLS,1);
     sg_read32s(fil,msx,4,sizeof(msx)/sizeof(msx[0]));
     sg_read32s(fil,msy,4,sizeof(msy)/sizeof(msy[0]));
     spriteqloc = sg_read16(fil);
#if DUKE3D_GAMEVER < GAMEVER_14
     // 1.3D's sprite queue is always 64 long and its length isn't saved.
     spriteqamount = 64;
     sg_read16s(fil,spriteq,2,64);
#else
     spriteqamount = sg_read16(fil);
     sg_read16sn(fil,spriteq,1024,2,spriteqamount);
     spriteqamount = clampcount(spriteqamount,1024);
#endif
     mirrorcnt = clampcount(sg_read16(fil),64);
     sg_read16s(fil,mirrorwall,2,64);
     sg_read16s(fil,mirrorsector,2,64);
#if DUKE3D_GAMEVER >= GAMEVER_10
     kdfread(&show2dsector[0],sizeof(char),MAXSECTORS>>3,fil);
#else
     memset(show2dsector,0,sizeof(show2dsector));
#endif
#if DUKE3D_GAMEVER >= GAMEVER_14 // 1.3D saves no actortype, boardfilename or clouds
     clearbufbyte(actortype,MAXTILES,0L);
     kdfread(&actortype[0],sizeof(char),SG_DOS_MAXTILES,fil);
     kdfread(&boardfilename[0],sizeof(boardfilename),1,fil);

     numclouds = clampcount(sg_read16(fil),128);
     sg_read16s(fil,clouds,sizeof(short)<<7,1);
     sg_read16s(fil,cloudx,sizeof(short)<<7,1);
     sg_read16s(fil,cloudy,sizeof(short)<<7,1);
#endif

     kdfread(&scriptptrs[0],1,SG_DOS_MAXSCRIPT,fil);
     for(i=0;i<SG_DOS_MAXSCRIPT;i++)
         if(scriptptrs[i]) { dos = 1; break; }
     sg_read32s(fil,script,4,SG_DOS_MAXSCRIPT);

     clearbuf(actorscrptr,MAXTILES,0L);
     sg_read32s(fil,actorscrptr,4,SG_DOS_MAXTILES);

#if DUKE3D_GAMEVER < GAMEVER_11
     // Every entry is stored minus the script's address, so the empty ones
     // hold minus that. An actor's temp_data[1], [4] and [5] at or above it
     // are addresses; sg_check10 has made sure they're in the script.
     for(i=0;i<SG_DOS_MAXTILES;i++)
         if(actorscrptr[i] == -bv) actorscrptr[i] = 0;
     for(i=0;i<SG_DOS_MAXHITTYPES;i++)
         if(sprite[i].picnum >= 0 && sprite[i].picnum < MAXTILES && actorscrptr[sprite[i].picnum])
             for(j=0;j<3;j++)
                 if(hittype[i].temp_data[sg_scriptslots10[j]] >= bv)
                     hittype[i].temp_data[sg_scriptslots10[j]] -= bv;
#elif DUKE3D_GAMEVER < GAMEVER_14
     clearbufbyte(hittype,sizeof(hittype),0L);
     kdfread(&scriptptrs[0],1,SG_DOS_MAXHITTYPES,fil);
     sg_readrecs(fil,hittype,sizeof(struct weaponhit),sg_weaponhit,SG_DOS_MAXHITTYPES,SG_WEAPONHIT_SIZE,SG_DOS_MAXHITTYPES);
#else
     kdfread(&scriptptrs[0],1,MAXSPRITES,fil);
     sg_readrecs(fil,hittype,sizeof(struct weaponhit),sg_weaponhit,MAXSPRITES,SG_WEAPONHIT_SIZE,MAXSPRITES);
#endif

     lockclock = sg_read32(fil);
     pskybits = sg_read16(fil);
     sg_read16s(fil,pskyoff,2,MAXPSKYTILES);

     animatecnt = clampcount(sg_read32(fil),MAXANIMATES);
     sg_read16s(fil,animatesect,2,MAXANIMATES);
     sg_read32s(fil,animoffs,4,MAXANIMATES);
     for(i = animatecnt-1;i>=0;i--) animateptr[i] = sg_animptr(animoffs[i],dos);
     sg_read32s(fil,animategoal,4,MAXANIMATES);
     sg_read32s(fil,animatevel,4,MAXANIMATES);

     earthquaketime = sg_read8(fil);
     ud.from_bonus = sg_read16(fil);
#if DUKE3D_GAMEVER < GAMEVER_10
     ud.respawn_monsters = (uint16_t)sg_read16(fil);
     ud.m_respawn_monsters = ud.respawn_monsters;
     ud.respawn_items = ud.m_respawn_items = 0;
#elif DUKE3D_GAMEVER < GAMEVER_13D
     // 1.1 ends with 7 scalars; auto_run and crosshair are bytes. 1.0 ends
     // with the first 4.
     ud.respawn_monsters = sg_read32(fil);
     ud.m_respawn_monsters = ud.respawn_monsters;
     ud.respawn_items = sg_read32(fil);
     ud.m_respawn_items = ud.respawn_items;
#if DUKE3D_GAMEVER >= GAMEVER_11
     ud.god = sg_read8(fil);
     ud.auto_run = sg_read8(fil);
     ud.crosshair = sg_read8(fil);
#endif
#else
     ud.secretlevel = sg_read16(fil);
     ud.respawn_monsters = sg_read32(fil);
     ud.m_respawn_monsters = ud.respawn_monsters;
     ud.respawn_items = sg_read32(fil);
     ud.m_respawn_items = ud.respawn_items;
     ud.respawn_inventory = sg_read32(fil);
     ud.m_respawn_inventory = ud.respawn_inventory;

     ud.god = sg_read8(fil);
     ud.auto_run = sg_read32(fil);
     ud.crosshair = sg_read32(fil);
     ud.monsters_off = sg_read32(fil);
     ud.m_monsters_off = ud.monsters_off;
     ud.last_level = sg_read16(fil);
     ud.eog = sg_read8(fil);
#endif

#if DUKE3D_GAMEVER >= GAMEVER_14 // a 1.3D save ends here
     ud.coop = sg_read32(fil);
     ud.m_coop = ud.coop;
     ud.marker = sg_read32(fil);
     ud.m_marker = ud.marker;
     ud.ffire = sg_read32(fil);
     ud.m_ffire = ud.ffire;

     camsprite = sg_read16(fil);
     connecthead = sg_read16(fil);
     sg_read16s(fil,connectpoint2,sizeof(connectpoint2),1);
     numplayersprites = sg_read8(fil);
     sg_read16s(fil,&frags[0][0],sizeof(frags),1);

     randomseed = sg_read32(fil);
     global_random = sg_read16(fil);
     parallaxyscale = sg_read32(fil);
#endif

     kclose(fil);

     if(ps[myconnectindex].over_shoulder_on != 0)
     {
         cameradist = 0;
         cameraclock = 0;
         ps[myconnectindex].over_shoulder_on = 1;
     }

     screenpeek = myconnectindex;

     clearbufbyte(gotpic,sizeof(gotpic),0L);
     clearsoundlocks();
         cacheit();
     docacheit();

     if(music_changed == 0)
        music_select = (ud.volume_number*11) + ud.level_number;
     playmusic(&music_fn[0][(int)music_select][0]);

     ps[myconnectindex].gm = MODE_GAME;
         ud.recstat = 0;

     if(ps[myconnectindex].jetpack_on)
         spritesound(DUKE_JETPACK_IDLE,ps[myconnectindex].i);

     restorepalette = 1;
     setpal(&ps[myconnectindex]);
     vscrn();

     FX_SetReverb(0);

     if(ud.lockout == 0)
     {
         for(x=0;x<numanimwalls;x++)
             if( wall[animwall[x].wallnum].extra >= 0 )
                 wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
     }
     else
     {
         for(x=0;x<numanimwalls;x++)
             switch(wall[animwall[x].wallnum].picnum)
         {
             case FEMPIC1:
                 wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                 break;
             case FEMPIC2:
             case FEMPIC3:
                 wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                 break;
         }
     }

     numinterpolations = 0;
     startofdynamicinterpolations = 0;

     k = headspritestat[3];
     while(k >= 0)
     {
        switch(sprite[k].lotag)
        {
            case 31:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                break;
            case 32:
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 25:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 17:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 0:
            case 5:
            case 6:
            case 11:
            case 14:
            case 15:
            case 16:
            case 26:
            case 30:
                setsectinterpolate(k);
                break;
        }

        k = nextspritestat[k];
     }

     for(i=numinterpolations-1;i>=0;i--) bakipos[i] = *curipos[i];
     for(i = animatecnt-1;i>=0;i--)
         setinterpolation(animateptr[i]);

     show_shareware = 0;
     everyothertime = 0;

     clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);

     /* Seed local prediction from the restored player before clearing input
      * history and synchronizing the restart. */
     resetmys();

     ready2send = 1;

     flushpackets();
     clearfifo();
     waitforeverybody();

     resettimevars();

     return(0);
}

/*
================
saveplayer

Resolve the save slot and multiplayer filename, coordinating with peers
where this game version requires it.
================
*/
int saveplayer(signed char spot)
{
     char fn[] = "game0.sav";
     char mpfn[] = "gameA_00.sav";
     char fullname[MAX_PATH];

     if(spot < 0)
     {
#if DUKE3D_GAMEVER < GAMEVER_14
        // 1.3D saves locally in a network game too; it has no save request.
        spot = -spot-1;
#else
        multiflag = 1;
        multiwhat = 1;
        multipos = -spot-1;
        return -1;
#endif
     }

#if DUKE3D_GAMEVER >= GAMEVER_14 // 1.3D's saveplayer doesn't wait for the other players
     waitforeverybody();
#endif

     if( multiflag == 2 && multiwho != myconnectindex )
     {
         mpfn[4] = spot + 'A';

         if(ud.multimode > 9)
         {
             mpfn[6] = (multiwho/10) + '0';
             mpfn[7] = multiwho + '0';
         }
         else mpfn[7] = multiwho + '0';

         GetPathFromEnvironment(fullname, sizeof(fullname), mpfn);
     }
     else
     {
        fn[4] = spot + '0';

        GetPathFromEnvironment(fullname, sizeof(fullname), fn);
     }

     return saveplayerfile(fullname,spot);
}

/*
================
saveplayerfile

Write the game state and menu preview using the selected version's DOS layout.
Store serialized references rather than native pointer values.
================
*/
int saveplayerfile(char *fnptr, signed char spot)
{
     int32_t i, j, animoffs[MAXANIMATES];
     char scriptptrs[MAXSCRIPTSIZE];
     FILE *fil;

#if DUKE3D_GAMEVER < GAMEVER_13D
     if(script[0] > SG_DOS_MAXSCRIPT*4)
         printf("Warning: DOS saves store only %d script longs; this CON uses %d.\n",SG_DOS_MAXSCRIPT,script[0]/4);
#endif
     if ((fil = fopen(fnptr,"wb")) == 0) return(-1);

     ready2send = 0;

#if DUKE3D_GAMEVER >= GAMEVER_11 // 1.0 saves have no version
     sg_write32(fil,BYTEVERSION);
#endif
#if DUKE3D_GAMEVER >= GAMEVER_14 // 1.3D saves have no player count
     sg_write32(fil,ud.multimode);
#endif

     dfwrite(&ud.savegame[spot][0],19,1,fil);
     sg_write32(fil,ud.volume_number);
     sg_write32(fil,ud.level_number);
     sg_write32(fil,ud.player_skill);
     dfwrite((char *)waloff[MAXTILES-1],160,100,fil);

     sg_write16(fil,numwalls);
#if DUKE3D_GAMEVER < GAMEVER_11 // 1.0 saves only the map's walls and sectors
     sg_writerecs(fil,wall,sizeof(walltype),sg_wall,numwalls,SG_WALL_SIZE,numwalls);
#else
     sg_writerecs(fil,wall,sizeof(walltype),sg_wall,MAXWALLS,SG_WALL_SIZE,MAXWALLS);
#endif
     sg_write16(fil,numsectors);
#if DUKE3D_GAMEVER < GAMEVER_11
     sg_writerecs(fil,sector,sizeof(sectortype),sg_sector,numsectors,SG_SECTOR_SIZE,numsectors);
#else
     sg_writerecs(fil,sector,sizeof(sectortype),sg_sector,MAXSECTORS,SG_SECTOR_SIZE,MAXSECTORS);
#endif
     sg_writerecs(fil,sprite,sizeof(spritetype),sg_sprite,MAXSPRITES,SG_SPRITE_SIZE,MAXSPRITES);
     sg_write16s(fil,headspritesect,2,MAXSECTORS+1);
     sg_write16s(fil,prevspritesect,2,MAXSPRITES);
     sg_write16s(fil,nextspritesect,2,MAXSPRITES);
     sg_write16s(fil,headspritestat,2,MAXSTATUS+1);
     sg_write16s(fil,prevspritestat,2,MAXSPRITES);
     sg_write16s(fil,nextspritestat,2,MAXSPRITES);
     sg_write16(fil,numcyclers);
     sg_write16s(fil,&cyclers[0][0],12,MAXCYCLERS);
     sg_writerecs(fil,ps,sizeof(struct player_struct),sg_player,SG_DOS_MAXPLAYERS,SG_PLAYER_DFSIZE,SG_PLAYER_DFCOUNT);
     sg_writerecs(fil,po,sizeof(struct player_orig),sg_playerorig,SG_DOS_MAXPLAYERS,SG_PLAYERORIG_DFSIZE,SG_PLAYERORIG_DFCOUNT);
#if DUKE3D_GAMEVER < GAMEVER_11
     // 1.0 saves hittype[] here, raw: an actor's script offsets in
     // temp_data[1], [4] and [5] go out as DOS addresses (F10-003), and
     // scriptptrs marks them to undo after. Native slots have no type tags:
     // a numeric value equal to an aligned script offset is ambiguous here.
     // Correctly distinguishing it requires tracking slot types at assignment,
     // including native actor code; the DOS save format has no such metadata.
     for(i=0;i<SG_DOS_MAXHITTYPES;i++)
         for(j=0;j<3;j++)
         {
             int32_t *t = &hittype[i].temp_data[sg_scriptslots10[j]];
             scriptptrs[i*3+j] = sprite[i].picnum >= 0 && sprite[i].picnum < MAXTILES &&
                 actorscrptr[sprite[i].picnum] && *t > 0 && *t < SG_DOS_MAXSCRIPT*4 && (*t&3) == 0;
             if(scriptptrs[i*3+j]) *t += SG_DOS10_SCRIPTBASE;
         }
     sg_writerecs(fil,hittype,sizeof(struct weaponhit),sg_weaponhit,SG_DOS_MAXHITTYPES,SG_WEAPONHIT_SIZE,SG_DOS_MAXHITTYPES);
     for(i=0;i<SG_DOS_MAXHITTYPES;i++)
         for(j=0;j<3;j++)
             if(scriptptrs[i*3+j]) hittype[i].temp_data[sg_scriptslots10[j]] -= SG_DOS10_SCRIPTBASE;
#endif
     sg_write16(fil,numanimwalls);
     sg_writerecs(fil,animwall,sizeof(struct animwalltype),sg_animwall,MAXANIMWALLS,SG_ANIMWALL_SIZE*MAXANIMWALLS,1);
     sg_write32s(fil,msx,4,sizeof(msx)/sizeof(msx[0]));
     sg_write32s(fil,msy,4,sizeof(msy)/sizeof(msy[0]));
     sg_write16(fil,spriteqloc);
#if DUKE3D_GAMEVER < GAMEVER_14
     sg_write16s(fil,spriteq,2,64);  // 1.3D's queue is always 64 long
#else
     sg_write16(fil,spriteqamount);
     sg_write16s(fil,spriteq,2,spriteqamount);
#endif
     sg_write16(fil,mirrorcnt);
     sg_write16s(fil,mirrorwall,2,64);
     sg_write16s(fil,mirrorsector,2,64);
#if DUKE3D_GAMEVER >= GAMEVER_10
     dfwrite(&show2dsector[0],sizeof(char),MAXSECTORS>>3,fil);
#endif
#if DUKE3D_GAMEVER >= GAMEVER_14 // 1.3D saves no actortype, boardfilename or clouds
     dfwrite(&actortype[0],sizeof(char),SG_DOS_MAXTILES,fil);
     dfwrite(&boardfilename[0],sizeof(boardfilename),1,fil);

     sg_write16(fil,numclouds);
     sg_write16s(fil,clouds,sizeof(short)<<7,1);
     sg_write16s(fil,cloudx,sizeof(short)<<7,1);
     sg_write16s(fil,cloudy,sizeof(short)<<7,1);
#endif

     // Script values are already offsets; the pointer flag blocks are kept zeroed.
     clearbufbyte(scriptptrs,SG_DOS_MAXSCRIPT,0L);

     dfwrite(&scriptptrs[0],1,SG_DOS_MAXSCRIPT,fil);
     sg_write32s(fil,script,4,SG_DOS_MAXSCRIPT);

#if DUKE3D_GAMEVER < GAMEVER_11
     // 1.0 stores every entry minus the script's address, the empty ones too.
     for(i=0;i<SG_DOS_MAXTILES;i++)
         if(actorscrptr[i] == 0) actorscrptr[i] = -SG_DOS10_SCRIPTBASE;
     sg_write32s(fil,actorscrptr,4,SG_DOS_MAXTILES);
     for(i=0;i<SG_DOS_MAXTILES;i++)
         if(actorscrptr[i] == -SG_DOS10_SCRIPTBASE) actorscrptr[i] = 0;
#else
     sg_write32s(fil,actorscrptr,4,SG_DOS_MAXTILES);
#endif

#if DUKE3D_GAMEVER < GAMEVER_11
     // 1.0 wrote hittype[] after po[].
#elif DUKE3D_GAMEVER < GAMEVER_14
     dfwrite(&scriptptrs[0],1,SG_DOS_MAXHITTYPES,fil);
     sg_writerecs(fil,hittype,sizeof(struct weaponhit),sg_weaponhit,SG_DOS_MAXHITTYPES,SG_WEAPONHIT_SIZE,SG_DOS_MAXHITTYPES);
#else
     dfwrite(&scriptptrs[0],1,MAXSPRITES,fil);
     sg_writerecs(fil,hittype,sizeof(struct weaponhit),sg_weaponhit,MAXSPRITES,SG_WEAPONHIT_SIZE,MAXSPRITES);
#endif

     sg_write32(fil,lockclock);
     sg_write16(fil,pskybits);
     sg_write16s(fil,pskyoff,2,MAXPSKYTILES);
     sg_write32(fil,animatecnt);
     sg_write16s(fil,animatesect,2,MAXANIMATES);
     clearbuf(animoffs,MAXANIMATES,0L);
     for(i = animatecnt-1;i>=0;i--) animoffs[i] = (int32_t)((char *)animateptr[i]-(char *)&sector[0]);
     sg_write32s(fil,animoffs,4,MAXANIMATES);
     sg_write32s(fil,animategoal,4,MAXANIMATES);
     sg_write32s(fil,animatevel,4,MAXANIMATES);

     sg_write8(fil,earthquaketime);
     sg_write16(fil,ud.from_bonus);
#if DUKE3D_GAMEVER < GAMEVER_10
     sg_write16(fil,ud.respawn_monsters);
#elif DUKE3D_GAMEVER < GAMEVER_13D
     sg_write32(fil,ud.respawn_monsters);
     sg_write32(fil,ud.respawn_items);
#if DUKE3D_GAMEVER >= GAMEVER_11 // 1.0 ends here
     sg_write8(fil,ud.god);
     sg_write8(fil,ud.auto_run);
     sg_write8(fil,ud.crosshair);
#endif
#else
     sg_write16(fil,ud.secretlevel);
     sg_write32(fil,ud.respawn_monsters);
     sg_write32(fil,ud.respawn_items);
     sg_write32(fil,ud.respawn_inventory);
     sg_write8(fil,ud.god);
     sg_write32(fil,ud.auto_run);
     sg_write32(fil,ud.crosshair);
     sg_write32(fil,ud.monsters_off);
     sg_write16(fil,ud.last_level);
     sg_write8(fil,ud.eog);
#endif
#if DUKE3D_GAMEVER >= GAMEVER_14 // a 1.3D save ends here
     sg_write32(fil,ud.coop);
     sg_write32(fil,ud.marker);
     sg_write32(fil,ud.ffire);
     sg_write16(fil,camsprite);
     sg_write16(fil,connecthead);
     sg_write16s(fil,connectpoint2,sizeof(connectpoint2),1);
     sg_write8(fil,numplayersprites);
     sg_write16s(fil,&frags[0][0],sizeof(frags),1);

     sg_write32(fil,randomseed);
     sg_write16(fil,global_random);
     sg_write32(fil,parallaxyscale);
#endif

         fclose(fil);

     if(ud.multimode < 2)
     {
         strcpy(fta_quotes[122],"GAME SAVED");
         FTA(122,&ps[myconnectindex]);
     }

     ready2send = 1;

#if DUKE3D_GAMEVER >= GAMEVER_14
     waitforeverybody();
#endif

     ototalclock = totalclock;

     return(0);
}

/*
 * -loadverify: load a save headless and print what the status bar shows.
 * With -saveverify, save it again straight away. Returns the exit code:
 * 0 loaded, 1 couldn't open or write a file, 2 refused.
 */
int loadverify(char *loadname, char *savename)
{
     struct player_struct *p = &ps[0];
     int r = loadplayerfile(loadname,0);

     if(r == -1)
     {
         printf("Can't open %s.\n",loadname);
         return 1;
     }
     if(r != 0) return 2;

     printf("SAVESTATE %d %d %d %d %d %d %d %d\n",
         sprite[p->i].extra,p->shield_amount,p->curr_weapon,
         p->ammo_amount[p->curr_weapon],ud.volume_number,ud.level_number,
         p->posx,p->posy);

     if(savename != NULL)
     {
         /* saveplayer writes the menu's screenshot tile; use the loaded one */
         walock[MAXTILES-3] = 255;
         walock[MAXTILES-1] = 255;
         if (waloff[MAXTILES-1] == 0) allocache(&waloff[MAXTILES-1],160*100,&walock[MAXTILES-1]);
         copybufbyte((void *)waloff[MAXTILES-3],(void *)waloff[MAXTILES-1],160*100);
         if(saveplayerfile(savename,0) != 0)
         {
             printf("Can't write %s.\n",savename);
             return 1;
         }
     }
     fflush(stdout);
     return 0;
}

#define LMB (buttonstat&1)
#define RMB (buttonstat&2)

ControlInfo minfo;

int32_t mi;

/*
================
probe_with_delete

Move and draw the menu cursor, then report selection or cancellation.
Return row >= 0 for activation or slider adjustment, -1 for cancel,
or -row-2 while browsing.
================
*/
static int probe_with_delete(int x,int y,int i,int n,int *hitdelete)
{
    short centre, s;

    s = 1+(CONTROL_GetMouseSensitivity()>>4);

    if( ControllerType == 1 && CONTROL_MousePresent )
    {
        CONTROL_GetInput( &minfo );
        /* Accumulate mouse movement until it crosses a menu-row threshold;
         * small motion must not select a new row every frame. */
        mi += minfo.dz;
    }

    else minfo.dz = minfo.dyaw = 0;

    if( x == (320>>1) )
        centre = 320>>2;
    else centre = 0;

    if(!buttonstat)
    {
        if( KB_KeyPressed( sc_UpArrow ) || KB_KeyPressed( sc_PgUp ) || KB_KeyPressed( sc_kpad_8 ) ||
            mi < -8192 )
        {
            mi = 0;
            KB_ClearKeyDown( sc_UpArrow );
            KB_ClearKeyDown( sc_kpad_8 );
            KB_ClearKeyDown( sc_PgUp );
            sound(KICK_HIT);

            probey--;
            if(probey < 0) probey = n-1;
            minfo.dz = 0;
        }
        if( KB_KeyPressed( sc_DownArrow ) || KB_KeyPressed( sc_PgDn ) || KB_KeyPressed( sc_kpad_2 )
            || mi > 8192 )
        {
            mi = 0;
            KB_ClearKeyDown( sc_DownArrow );
            KB_ClearKeyDown( sc_kpad_2 );
            KB_ClearKeyDown( sc_PgDn );
            sound(KICK_HIT);
            probey++;
            minfo.dz = 0;
        }
    }

    if(probey >= n)
        probey = 0;

    if(centre)
    {
//        rotatesprite(((320>>1)+(centre)+54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
//        rotatesprite(((320>>1)-(centre)-54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);

        rotatesprite(((320>>1)+(centre>>1)+70)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(((320>>1)-(centre>>1)-70)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);
    }
    else
        rotatesprite((x-tilesizx[BIGFNTCURSOR]-4)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+(((totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);

    /* Binding pages need deletion to return the selected row through the
     * same path as activation, with a separate flag identifying the action. */
    if (hitdelete != NULL)
    {
        *hitdelete = ((KB_KeyPressed(sc_Delete)) ||
                      (KB_KeyPressed(sc_BackSpace)) ||
                      (KB_KeyPressed(sc_kpad_Period)));

        if (*hitdelete)
        {
            sound(KICK_HIT);
            KB_ClearKeyDown(sc_Delete);
            KB_ClearKeyDown(sc_BackSpace);
            KB_ClearKeyDown(sc_kpad_Period);
            return(probey);
        }
    }

    if( KB_KeyPressed(sc_Space) || KB_KeyPressed( sc_kpad_Enter ) || KB_KeyPressed( sc_Enter ) || (LMB && !onbar) )
    {
        if(current_menu != 110)
            sound(PISTOL_BODYHIT);
        KB_ClearKeyDown( sc_Enter );
        KB_ClearKeyDown( sc_Space );
        KB_ClearKeyDown( sc_kpad_Enter );
        return(probey);
    }
    else if( KB_KeyPressed( sc_Escape ) || (RMB) )
    {
        onbar = 0;
        KB_ClearKeyDown( sc_Escape );
        sound(EXITMENUSOUND);
        return(-1);
    }
    else
    {
        /* Keep -1 reserved for cancellation; encode an idle row below it. */
        if(onbar == 0) return(-probey-2);
        if ( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -128 ) )
            return(probey);
        else if ( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 128 ) )
            return(probey);
        else return(-probey-2);
    }
}

/*
================
probe

Run the standard menu cursor without the optional delete-binding action.
================
*/
int probe(int x,int y,int i,int n)
{
    return probe_with_delete(x,y,i,n,NULL);
}

/*
================
menutext

Draw the large menu font in logical screen coordinates and return the end X.
An X of 160 requests centering by the measured glyph widths.
================
*/
int menutext(int x,int y,short s,short p,char *t)
{
    short i, ac, centre;

    y -= 12;

    i = centre = 0;

    /* Measure the glyphs first; menu text uses variable tile widths. */
    if( x == (320>>1) )
    {
        while( *(t+i) )
        {
            if(*(t+i) == ' ')
            {
                centre += 5;
                i++;
                continue;
            }
            ac = 0;
            if(*(t+i) >= '0' && *(t+i) <= '9')
                ac = *(t+i) - '0' + BIGALPHANUM-10;
            else if(*(t+i) >= 'a' && *(t+i) <= 'z')
                ac = toupper(*(t+i)) - 'A' + BIGALPHANUM;
            else if(*(t+i) >= 'A' && *(t+i) <= 'Z')
                ac = *(t+i) - 'A' + BIGALPHANUM;
            else switch(*(t+i))
            {
                case '-':
                    ac = BIGALPHANUM-11;
                    break;
                case '.':
                    ac = BIGPERIOD;
                    break;
                case '\'':
                    ac = BIGAPPOS;
                    break;
                case ',':
                    ac = BIGCOMMA;
                    break;
                case '!':
                    ac = BIGX;
                    break;
                case '?':
                    ac = BIGQ;
                    break;
                case ';':
                    ac = BIGSEMI;
                    break;
                case ':':
                    ac = BIGSEMI;
                    break;
                default:
                    centre += 5;
                    i++;
                    continue;
            }

            centre += tilesizx[ac]-1;
            i++;
        }
    }

    if(centre)
        x = (320-centre-10)>>1;

    while(*t)
    {
        if(*t == ' ') {x+=5;t++;continue;}
        ac = 0;
        if(*t >= '0' && *t <= '9')
            ac = *t - '0' + BIGALPHANUM-10;
        else if(*t >= 'a' && *t <= 'z')
            ac = toupper(*t) - 'A' + BIGALPHANUM;
        else if(*t >= 'A' && *t <= 'Z')
            ac = *t - 'A' + BIGALPHANUM;
        else switch(*t)
        {
            case '-':
                ac = BIGALPHANUM-11;
                break;
            case '.':
                ac = BIGPERIOD;
                break;
            case ',':
                ac = BIGCOMMA;
                break;
            case '!':
                ac = BIGX;
                break;
            case '\'':
                ac = BIGAPPOS;
                break;
            case '?':
                ac = BIGQ;
                break;
            case ';':
                ac = BIGSEMI;
                break;
            case ':':
                ac = BIGCOLIN;
                break;
            default:
                x += 5;
                t++;
                continue;
        }

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,10+16,0,0,xdim-1,ydim-1);

        x += tilesizx[ac];
        t++;
    }
    return (x);
}


/*
================
menutextc

Draw the alternate large-font text centered on X and return the end X.
================
*/
int menutextc(int x,int y,short s,short p,char *t)
{
    short i, ac, centre;

    s += 8;
    y -= 12;

    i = centre = 0;

//    if( x == (320>>1) )
    {
        while( *(t+i) )
        {
            if(*(t+i) == ' ')
            {
                centre += 5;
                i++;
                continue;
            }
            ac = 0;
            if(*(t+i) >= '0' && *(t+i) <= '9')
                ac = *(t+i) - '0' + BIGALPHANUM+26+26;
            if(*(t+i) >= 'a' && *(t+i) <= 'z')
                ac = *(t+i) - 'a' + BIGALPHANUM+26;
            if(*(t+i) >= 'A' && *(t+i) <= 'Z')
                ac = *(t+i) - 'A' + BIGALPHANUM;

            else switch(*t)
            {
                case '-':
                    ac = BIGALPHANUM-11;
                    break;
                case '.':
                    ac = BIGPERIOD;
                    break;
                case ',':
                    ac = BIGCOMMA;
                    break;
                case '!':
                    ac = BIGX;
                    break;
                case '?':
                    ac = BIGQ;
                    break;
                case ';':
                    ac = BIGSEMI;
                    break;
                case ':':
                    ac = BIGCOLIN;
                    break;
            }

            centre += tilesizx[ac]-1;
            i++;
        }
    }

    x -= centre>>1;

    while(*t)
    {
        if(*t == ' ') {x+=5;t++;continue;}
        ac = 0;
        if(*t >= '0' && *t <= '9')
            ac = *t - '0' + BIGALPHANUM+26+26;
        if(*t >= 'a' && *t <= 'z')
            ac = *t - 'a' + BIGALPHANUM+26;
        if(*t >= 'A' && *t <= 'Z')
            ac = *t - 'A' + BIGALPHANUM;
        switch(*t)
        {
            case '-':
                ac = BIGALPHANUM-11;
                break;
            case '.':
                ac = BIGPERIOD;
                break;
            case ',':
                ac = BIGCOMMA;
                break;
            case '!':
                ac = BIGX;
                break;
            case '?':
                ac = BIGQ;
                break;
            case ';':
                ac = BIGSEMI;
                break;
            case ':':
                ac = BIGCOLIN;
                break;
        }

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,10+16,0,0,xdim-1,ydim-1);

        x += tilesizx[ac];
        t++;
    }
    return (x);
}


/*
================
bar

Draw a menu slider and optionally change its value from directional input.
A negative step reverses both the control direction and knob placement.
================
*/
void bar(int x,int y,short *p,short dainc,char damodify,short s, short pa)
{
    short xloc;
    char rev;

    /* Reversed sliders still store a positive value; only input and drawing
     * run in the opposite direction. */
    if(dainc < 0) { dainc = -dainc; rev = 1; }
    else rev = 0;
    y-=2;

    if(damodify)
    {
        if(rev == 0)
        {
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ) ) // && onbar) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );

                *p -= dainc;
                if(*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ) )//&& onbar) )
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );

                *p += dainc;
                if(*p > 63)
                    *p = 63;
                sound(KICK_HIT);
            }
        }
        else
        {
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ))//&& onbar ))
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );

                *p -= dainc;
                if(*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ))// && onbar) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );

                *p += dainc;
                if(*p > 64)
                    *p = 64;
                sound(KICK_HIT);
            }
        }
    }

    xloc = *p;

    rotatesprite( (x+22)<<16,(y-3)<<16,65536L,0,SLIDEBAR,s,pa,10,0,0,xdim-1,ydim-1);
    if(rev == 0)
        rotatesprite( (x+xloc+1)<<16,(y+1)<<16,65536L,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
    else
        rotatesprite( (x+(65-xloc) )<<16,(y+1)<<16,65536L,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
}

#define SHX(X) 0
// ((x==X)*(-sh))
#define PHX(X) 0
// ((x==X)?1:2)
#define MWIN(X) rotatesprite( 320<<15,200<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)
#define MWINXY(X,OX,OY) rotatesprite( ( 320+(OX) )<<15, ( 200+(OY) )<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)

// The VIDEO page's RESOLUTION row: the size picked with the arrows, not yet
// applied (videopage_browse).
#define VIDEOPAGE_MAXMODES 256
#define VIDEOPAGE_MAXRES 64
static int32 videopage_browse, videopage_bw, videopage_bh;

// The next of count choices from cur in direction dir that avail allows,
// round and round; cur when there's no other.
static int videopage_next(int cur, int dir, int count, const int *avail)
{
    int i, c = cur;

    for(i=1;i<count;i++)
    {
        c = (c+dir+count)%count;
        if(avail[c]) return c;
    }
    return cur;
}

// The RESOLUTION row's sizes in display mode display, smallest first
// (videoopts_buildreslist, from the main display's modes); windowed, with
// the run's size w by h (if not 0).
static int videopage_reslist(int display, int32_t w, int32_t h, vmode_t *res)
{
    static int32_t mw[VIDEOPAGE_MAXMODES], mh[VIDEOPAGE_MAXMODES];
    static vmode_t modes[VIDEOPAGE_MAXMODES];
    int32_t dw, dh;
    int i, n = getvideomodelist(mw,mh,VIDEOPAGE_MAXMODES,&dw,&dh);

    for(i=0;i<n;i++)
    {
        modes[i].w = mw[i];
        modes[i].h = mh[i];
    }
    n = videoopts_buildreslist(modes,n,dw,dh,display,res,VIDEOPAGE_MAXRES);
    if(display == DISPLAY_WINDOWED && w > 0 && n < VIDEOPAGE_MAXRES)
    {
        for(i=0;i<n && (res[i].w < w || (res[i].w == w && res[i].h < h));i++);
        if(i == n || res[i].w != w || res[i].h != h)
        {
            memmove(&res[i+1],&res[i],(n-i)*sizeof(res[0]));
            res[i].w = w;
            res[i].h = h;
            n++;
        }
    }
    return n;
}

// The size one step in direction dir from w by h among res's n (not in it:
// the nearest that way); its index.
static int videopage_resstep(const vmode_t *res, int n, int32_t w, int32_t h, int dir)
{
    int i;

    for(i=0;i<n && (res[i].w < w || (res[i].w == w && res[i].h < h));i++);
    if(i < n && res[i].w == w && res[i].h == h) i += dir;
    else if(dir < 0) i--;
    if(i < 0) i = 0;
    if(i >= n) i = n-1;
    return i;
}

// The size w by h keeps going to display mode display: in fullscreen one of
// the main display's modes (this one, else the desktop's, else the
// largest); windowed, this one if it fits the desktop, else the largest
// that does.
static void videopage_modesize(int display, int32_t *w, int32_t *h)
{
    vmode_t res[VIDEOPAGE_MAXRES];
    int32_t dw, dh;
    int i, n = videopage_reslist(display,0,0,res);

    getvideomodelist(NULL,NULL,0,&dw,&dh);
    if(n == 0 || display == DISPLAY_BORDERLESS) return;
    if(display == DISPLAY_WINDOWED && *w <= dw && *h <= dh) return;
    for(i=0;i<n;i++)
        if(res[i].w == *w && res[i].h == *h) return;
    for(i=0;i<n;i++)
        if(res[i].w == dw && res[i].h == dh) break;
    if(i == n) i = n-1;
    *w = res[i].w;
    *h = res[i].h;
}


int32 volnum,levnum,plrskl,numplr;
short lastsavedpos = -1;

/*
================
dispnames

Draw the ten save-slot names inside the load/save menu frame.
================
*/
void dispnames(void)
{
    short x, c = 160;

    c += 64;
    for(x = 0;x <= 108;x += 12)
    rotatesprite((c+91-64)<<16,(x+56)<<16,65536L,0,TEXTBOX,24,0,10,0,0,xdim-1,ydim-1);

    rotatesprite(22<<16,97<<16,65536L,0,WINDOWBORDER2,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(180<<16,97<<16,65536L,1024,WINDOWBORDER2,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(99<<16,50<<16,65536L,512,WINDOWBORDER1,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(103<<16,144<<16,65536L,1024+512,WINDOWBORDER1,24,0,10,0,0,xdim-1,ydim-1);

    minitext(c,48,ud.savegame[0],2,10+16);
    minitext(c,48+12,ud.savegame[1],2,10+16);
    minitext(c,48+12+12,ud.savegame[2],2,10+16);
    minitext(c,48+12+12+12,ud.savegame[3],2,10+16);
    minitext(c,48+12+12+12+12,ud.savegame[4],2,10+16);
    minitext(c,48+12+12+12+12+12,ud.savegame[5],2,10+16);
    minitext(c,48+12+12+12+12+12+12,ud.savegame[6],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12,ud.savegame[7],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12+12,ud.savegame[8],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12+12+12,ud.savegame[9],2,10+16);

}


// This is the same thing as was in build.c ...
//  We DO have a _dos_findfirst implementation now...maybe use that instead?
//     --ryan.
#if ORIGINAL_DUKE3D_GETFILENAMES
int getfilenames(char kind[6])
{
        short type;
        struct find_t fileinfo;

        if (strcmp(kind,"SUBD") == 0)
        {
                strcpy(kind,"*.*");
                if (_dos_findfirst(kind,_A_SUBDIR,&fileinfo) != 0)
                        return(-1);
                type = 1;
        }
        else
        {
                if (_dos_findfirst(kind,_A_NORMAL,&fileinfo) != 0)
                        return(-1);
                type = 0;
        }
        do
        {
                if ((type == 0) || ((fileinfo.attrib&16) > 0))
                        if ((fileinfo.name[0] != '.') || (fileinfo.name[1] != 0))
                        {
                                strcpy(menuname[menunamecnt],fileinfo.name);
                                menuname[menunamecnt][16] = type;
                                menunamecnt++;
                        }
        }
        while (_dos_findnext(&fileinfo) == 0);

        return(0);
}

#else

int getfilenames(char kind[6])
{
/* !!! FIXME: Visual C? */
#if   (defined PLATFORM_UNIX)

    DIR *dir;
    struct dirent *dent;
    struct stat statbuf;
    int add_this;
    char *ptr = NULL;
    int len = 0;
    int subdirs = 0;

	if (strcmp(kind,"SUBD") == 0)
        subdirs = 1;

    dir = opendir(".");
    if (dir == NULL)
        return(-1);

    do
    {
        add_this = 0;
        dent = readdir(dir);
        if (dent != NULL)
        {
#ifndef DC
            if (stat(dent->d_name, &statbuf) == 0)
#endif
            {
                if (subdirs)
                {
#ifdef DC
		    if (dent->size==-1)
#else
                    if (S_ISDIR(statbuf.st_mode))
#endif
                    add_this = 1;
                } /* if */
                else
                {
                    /* need to expand support if this assertion ever fails. */
                    assert(stricmp(kind, "*.MAP") == 0);
                    len = (int)strlen(dent->d_name);
                    if (len >= 5)
                    {
                        ptr = ((char *) dent->d_name) + len;
                        ptr += strlen(ptr) - 4;
                        if (stricmp(ptr, ".MAP") == 0)
                    add_this = 1;
                    } /* if */
                } /* else */

                if (add_this)
    			{
	    			strcpy(menuname[(int)menunamecnt],dent->d_name);
		    		menuname[(int)menunamecnt][16] = subdirs;
			    	menunamecnt++;
                } /* if */
            } /* if */
        } /* if */
    } while (dent != NULL);

    closedir(dir);

#endif
	return(0);
}

#endif


/*
================
sortfilenames

Sort the menu filename records in bytewise name order,
keeping each record's file/directory marker with its name.
================
*/
void sortfilenames()
{
        char sortbuffer[17];
        int32_t i, j, k;

        for(i=1;i<menunamecnt;i++)
                for(j=0;j<i;j++)
                {
                         k = 0;
                         while ((menuname[i][k] == menuname[j][k]) && (menuname[i][k] != 0) && (menuname[j][k] != 0))
                                 k++;
                        if (menuname[i][k] < menuname[j][k])
                        {
                                memcpy(&sortbuffer[0],&menuname[i][0],sizeof(menuname[0]));
                                memcpy(&menuname[i][0],&menuname[j][0],sizeof(menuname[0]));
                                memcpy(&menuname[j][0],&sortbuffer[0],sizeof(menuname[0]));
                        }
                }
}

int32_t quittimer = 0;


#define MENU_CONTROLS 30000
#define MENU_CONTROLS_MOVEMENT 30001
#define MENU_CONTROLS_MOREMOVEMENT 30002
#define MENU_CONTROLS_ACTIONS 30003
#define MENU_CONTROLS_WEAPONS 30004
#define MENU_CONTROLS_ITEMS 30005
#define MENU_CONTROLS_MISC 30006
#define MENU_CONTROLS_KEYBINDER 30007

#if CONTROLS_CONFIG_MENU

static int controlsstackedprobey = -1;

#define ALEN(x) (sizeof (x) / sizeof (x[0]))

typedef struct
{
    char *name;
    int32 which;
} KeyBinds;

/*
================
keybindtext

Draw one control action and the current name of its keyboard mapping.
================
*/
static void keybindtext(int pos, char *name, int32 which)
{
    char *binding = (char *) CONTROL_GetMappingName(which);
    if (binding == NULL)
        binding = "?";
    pos++;
    menutext(45,20+(16*pos),SHX(-pos),PHX(-pos),name);
    menutext(235,20+(16*pos),SHX(-pos),PHX(-pos),binding);
}


static KeyBinds *currentbind = NULL;
static int prevbindmenu = 0;
static int keybindprobey = 0;

/*
================
DoKeybindMenu

Browse a binding group, clear a selected mapping, or enter key capture.
Remember the previous page and cursor row for the return trip.
================
*/
static void DoKeybindMenu(char *name, KeyBinds *keybinds, int cnt, int prevmenu)
{
    int i;
    int rc;
    int hitdelete = 0;
    int drawmenu = 0;
    rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
    menutext(320>>1,15,0,0,name);

    rc = probe_with_delete(46,20+16,16,cnt,&hitdelete);
    if (rc == -1) // escape
    {
        cmenu(prevmenu);
        if (controlsstackedprobey >= 0)
        {
            probey = controlsstackedprobey;
            controlsstackedprobey = -1;
        }
    }
    else if (rc >= 0)  // chose an option.
    {
        if (hitdelete)   // clear this keybind.
        {
            char *fname = CONFIG_FunctionNumToName(keybinds[rc].which);
            CONTROL_MapKey(keybinds[rc].which, 0, 0);
            CONFIG_PutString("KeyDefinitions", fname, "");
            drawmenu = 1;
        }
        else   // Go to the keybinder menu to select a new key.
        {
            prevbindmenu = current_menu;
            currentbind = &keybinds[rc];
            keybindprobey = probey;
            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();
            cmenu(MENU_CONTROLS_KEYBINDER);
        }
    }
    else  // unknown option or none chosen, so draw menu.
    {
        drawmenu = 1;
    }

    if (drawmenu)
    {
        for (i = 0; i < cnt; i++)
            keybindtext(i, keybinds[i].name, keybinds[i].which);
    }
}
#endif

/*
================
non3dr_menus

Dispatch the port-added control configuration pages and key capture state.
================
*/
static void non3dr_menus(void)
{
    short c = (320>>1);
    int rc;

    switch(current_menu)
    {
#if CONTROLS_CONFIG_MENU
        case MENU_CONTROLS_KEYBINDER:  // binding a new key.
        {
            int bound = 0;
            char *binding;
            if (currentbind == NULL)  // sanity check.
            {
                cmenu(prevbindmenu ? prevbindmenu : MENU_CONTROLS);
                break;
            }

            if (KB_KeyWaiting())
            {
                kb_scancode sc = KB_GetLastScanCode();
                char *scstr = KB_ScanCodeToString(sc);
                KB_FlushKeyboardQueue();
                KB_ClearKeysDown();

                if (scstr == NULL)  // make sure it maps.
                    sound(KICK_HIT);
                else
                {
                    if (sc == sc_Escape)  // cancel request?
                        sound(EXITMENUSOUND);
                    else
                    {
                        char *fname = CONFIG_FunctionNumToName(currentbind->which);
                        sound(PISTOL_BODYHIT);
                        CONTROL_MapKey(currentbind->which, sc, 0);
                        CONFIG_PutString("KeyDefinitions", fname, scstr);
                    }
                    bound = 1;
                }
            }

            #if 0  // !!! FIXME...yuck.
            else
            {
                int32 buttons = MOUSE_GetButtons();
                if (buttons)
                {
                    int i;
                    int32 b = 1;
                    for (i = 0; i < MAXMOUSEBUTTONS; i++, b <<= 1)
                    {
                        if (b & buttons)
                        {
                            char *fname = CONFIG_FunctionNumToName(currentbind->which);
                            char buf[64];
                            snprintf(buf, sizeof (buf), "MouseButton%d", i);
                            CONTROL_MapButton(currentbind->which, i, 0);
                            CONFIG_PutString("Controls", buf, fname);
                            break;
                        }
                    }
                    bound = ((b & buttons) != 0);
                }
            }
            #endif

            if (bound)
            {
                cmenu(prevbindmenu ? prevbindmenu : MENU_CONTROLS);
                prevbindmenu = 0;
                currentbind = NULL;
                probey = keybindprobey;
                break;  // we're done.
            }

            // still waiting for valid user input...draw UI.

            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,currentbind->name);

            binding = (char *) CONTROL_GetMappingName(currentbind->which);
            if (binding != NULL)
            {
                menutext(320>>1,15+30,0,0,"CURRENTLY BOUND TO:");
                menutext(320>>1,15+30+16,0,0,binding);
            }
            else
            {
                menutext(320>>1,15+30+16,0,0,"NOT CURRENTLY BOUND");
            }

            menutext(320>>1,15+30+16+30,0,0,"PRESS NEW KEY");
            menutext(320>>1,15+30+16+30+16,0,0,"OR");
            menutext(320>>1,15+30+16+30+16+16,0,0,"ESC TO LEAVE AS IS");

            rotatesprite((320>>1)<<16,(15+30+16+30+16+16+20)<<16,65536L,0,SPINNINGNUKEICON+(((totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
            
            break;
        }

        case MENU_CONTROLS:  // main config menu.
        {
            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"CONTROLS");

            ignore_escape = 1;
            rc = probe(320/4,20+16,16,6);  // !!! FIXME: Move cursor to right more.
            controlsstackedprobey = rc;
            switch (rc)
            {
                case -1: // escape
                    cmenu(200);  // options menu
                    ignore_escape = 0;
                    break;
                case 0:  // movement
                    cmenu(MENU_CONTROLS_MOVEMENT);
                    break;
                case 1:  // more movement
                    cmenu(MENU_CONTROLS_MOREMOVEMENT);
                    break;
                case 2:  // actions
                    cmenu(MENU_CONTROLS_ACTIONS);
                    break;
                case 3:  // weapons
                    cmenu(MENU_CONTROLS_WEAPONS);
                    break;
                case 4:  // items
                    cmenu(MENU_CONTROLS_ITEMS);
                    break;
                case 5:  // misc
                    cmenu(MENU_CONTROLS_MISC);
                    break;
                default:
                    menutext(c,20+16,SHX(-1),PHX(-1),"MOVEMENT");
                    menutext(c,20+16+16,SHX(-2),PHX(-2),"MORE MOVEMENT");
                    menutext(c,20+16+16+16,SHX(-3),PHX(-3),"ACTIONS");
                    menutext(c,20+16+16+16+16,SHX(-4),PHX(-4),"WEAPON SELECT");
                    menutext(c,20+16+16+16+16+16,SHX(-5),PHX(-5),"INVENTORY");
                    menutext(c,20+16+16+16+16+16+16,SHX(-6),PHX(-6),"MISCELLANEOUS");
                    break;
            }
            break;
        }

        case MENU_CONTROLS_MOVEMENT:  // movement controls.
        {
            static KeyBinds keybinds[] =
            {
                { "MOVE FORWARD", gamefunc_Move_Forward },
                { "MOVE BACKWARD", gamefunc_Move_Backward },
                { "STRAFE LEFT", gamefunc_Strafe_Left },
                { "STRAFE RIGHT", gamefunc_Strafe_Right },
                { "TURN LEFT",gamefunc_Turn_Left },
                { "TURN RIGHT",gamefunc_Turn_Right },
                { "STRAFE", gamefunc_Strafe }
            };

            DoKeybindMenu("MOVEMENT", keybinds, ALEN(keybinds), MENU_CONTROLS);
            break;
        }

        case MENU_CONTROLS_MOREMOVEMENT:  // more movement controls.
        {
            static KeyBinds keybinds[] =
            {
                { "JUMP", gamefunc_Jump },
                { "CROUCH", gamefunc_Crouch },
                { "RUN", gamefunc_Run },
                { "RUNLOCK", gamefunc_AutoRun },
                { "AIM UP", gamefunc_Aim_Up },
                { "AIM DOWN", gamefunc_Aim_Down },
                { "CENTER VIEW", gamefunc_Center_View },
#if DUKE3D_GAMEVER >= GAMEVER_11 // 1.0 has no mouse aiming
                { "MOUSE AIM", gamefunc_Mouse_Aiming }
#endif
            };

            DoKeybindMenu("MORE MOVEMENT", keybinds, ALEN(keybinds), MENU_CONTROLS);
            break;
        }

        case MENU_CONTROLS_ACTIONS:   // actions controls
        {
            static KeyBinds keybinds[] =
            {
                { "FIRE OR ATTACK", gamefunc_Fire },
                { "OPEN OR USE", gamefunc_Open },
                { "QUICK KICK", gamefunc_Quick_Kick },
                { "180 TURN", gamefunc_TurnAround },
                { "VIEW MAP", gamefunc_Map },
                { "HOLSTER WEAPON" ,gamefunc_Holster_Weapon },
                { "PREVIOUS WEAPON", gamefunc_Previous_Weapon },
                { "NEXT WEAPON", gamefunc_Next_Weapon }
            };

            DoKeybindMenu("ACTIONS", keybinds, ALEN(keybinds), MENU_CONTROLS);
            break;
        }

        case MENU_CONTROLS_WEAPONS:   // weapon controls
        {
            static KeyBinds keybinds[] =
            {
                { "KICK", gamefunc_Weapon_1 },
                { "PISTOL", gamefunc_Weapon_2 },
                { "SHOTGUN", gamefunc_Weapon_3 },
                { "RIPPER", gamefunc_Weapon_4 },
                { "RPG", gamefunc_Weapon_5 },
                { "PIPEBOMB", gamefunc_Weapon_6 },
                { "SHRINK RAY", gamefunc_Weapon_7 },
                { "DEVASTATOR", gamefunc_Weapon_8 },
                { "TRIP BOMP", gamefunc_Weapon_9 },
                { "FREEZER", gamefunc_Weapon_10 }
            };

            DoKeybindMenu("WEAPON SELECT", keybinds, ALEN(keybinds), MENU_CONTROLS);
            break;
        }

        case MENU_CONTROLS_ITEMS:   // item controls
        {
            static KeyBinds keybinds[] =
            {
                { "INVENTORY", gamefunc_Inventory },
                { "PREVIOUS ITEM", gamefunc_Inventory_Left },
                { "NEXT ITEM", gamefunc_Inventory_Right },
                { "USE HOLODUKE", gamefunc_Holo_Duke},
                { "USE JETPACK", gamefunc_Jetpack },
                { "USE NIGHTVISION", gamefunc_NightVision },
                { "USE MEDKIT", gamefunc_MedKit },
#if DUKE3D_GAMEVER >= GAMEVER_11 // 1.0 sends no steroids bit (G10-S06)
                { "USE STEROIDS", gamefunc_Steroids }
#endif
            };

            DoKeybindMenu("INVENTORY", keybinds, ALEN(keybinds), MENU_CONTROLS);
            break;
        }

        case MENU_CONTROLS_MISC:   // misc controls
        {
            static KeyBinds keybinds[] =
            {
                { "SHRINK SCREEN", gamefunc_Shrink_Screen },
                { "ENLARGE SCREEN", gamefunc_Enlarge_Screen },
                { "SHOW WEAPONS", gamefunc_Show_Opponents_Weapon },
                { "MAP FOLLOW MODE", gamefunc_Map_Follow_Mode },
                { "SEE COOP VIEW", gamefunc_See_Coop_View },
                { "CHAT MESSAGE", gamefunc_SendMessage },
#if DUKE3D_GAMEVER >= GAMEVER_11 // 1.0 has no crosshair (G10-G07)
                { "TOGGLE CROSSHAIR", gamefunc_Toggle_Crosshair },
#endif
            };

            DoKeybindMenu("MISCELLANEOUS", keybinds, ALEN(keybinds), MENU_CONTROLS);
            break;
        }
#endif

        default:
            //gameexit("\nERROR: Unknown menu?!\n");
            cmenu(0);  // main menu
    }
}


#if DUKE3D_GAMEVER < GAMEVER_11
// 1.0's credits: six pages, with no hardware page, as its menus() draws them
// (page, y, line) (G10-G08).
#if DUKE3D_GAMEVER < GAMEVER_10
static const struct { short page, y; const char *text; } credits099[] =
{
    { 0, 50, "ORIGINAL CONCEPT BY" },
    { 0, 59, "TODD J REPLOGLE" },
    { 0, 68, "ALLEN H BLUM III" },
    { 0, 90, "PRODUCED & DIRECTED BY" },
    { 0, 99, "GREG MALONE" },
    { 0, 121, "EXECUTIVE PRODUCER" },
    { 0, 130, "GEORGE BROUSSARD" },
    { 1, 50, "GAME PROGRAMMED BY" },
    { 1, 59, "TODD REPLOGLE" },
    { 1, 80, "BUILD 3D ENGINE" },
    { 1, 89, "KEN SILVERMAN" },
    { 1, 111, "SUPPORT PROGRAMMING" },
    { 1, 120, "MARK DOCHTERMANN" },
    { 1, 129, "JIM DOSE" },
    { 2, 50, "MAP DEVELOPMENT" },
    { 2, 59, "ALLEN H BLUM III" },
    { 2, 68, "RICHARD GRAY" },
    { 2, 90, "3D ACTOR MODELING" },
    { 2, 99, "CHUCK JONES" },
    { 2, 108, "MIKE TIDWELL" },
    { 2, 117, "MARCUS FISHER" },
    { 3, 40, "PIXEL ART" },
    { 3, 49, "DIRK JONES" },
    { 3, 58, "STEPHEN HORNBACK" },
    { 3, 67, "JAMES STOREY" },
    { 3, 89, "SOUND & MUSIC DEVELOPMENT" },
    { 3, 98, "ROBERT PRINCE" },
    { 3, 107, "LEE JACKSON" },
    { 3, 129, "VOICE DEVELOPMENT" },
    { 3, 138, "LANI MINELLA" },
    { 3, 147, "DUKE NUKEM" },
    { 4, 50, "SPECIAL THANKS TO" },
    { 4, 68, "STEVEN BLACKBURN" },
    { 4, 77, "JOE SIEGLER" },
    { 4, 86, "TERRY NAGY" },
    { 5, 100, "MADE IN DALLAS, TEXAS USA" },
    { 6, 94, "DUKE NUKEM IS A TRADEMARK OF" },
    { 6, 103, "3D REALMS ENTERTAINMENT" },
    { 6, 116, "DUKE NUKEM 3D" },
    { 6, 125, "(C) 1996 3D REALMS ENTERTAINMENT" },
};
#endif
static const struct { short page, y; const char *text; } credits10[] =
{
    { 0, 50, "ORIGINAL CONCEPT" },
    { 0, 59, "TODD REPLOGLE" },
    { 0, 68, "ALLEN H. BLUM III" },
    { 0, 90, "PRODUCED & DIRECTED BY" },
    { 0, 99, "GREG MALONE" },
    { 0, 121, "EXECUTIVE PRODUCER" },
    { 0, 130, "GEORGE BROUSSARD" },
    { 1, 50, "GAME PROGRAMMING" },
    { 1, 59, "TODD REPLOGLE" },
    { 1, 80, "BUILD ENGINE/TOOLS/NET/MODEM" },
    { 1, 89, "KEN SILVERMAN" },
    { 1, 111, "MAP DESIGN" },
    { 1, 120, "ALLEN H BLUM III" },
    { 1, 129, "RICHARD GRAY" },
    { 2, 35, "3D MODELING" },
    { 2, 44, "CHUCK JONES" },
    { 2, 53, "SAPPHIRE CORPORATION" },
    { 2, 73, "ARTWORK" },
    { 2, 82, "DIRK JONES" },
    { 2, 91, "STEPHEN HORNBACK" },
    { 2, 100, "JAMES STOREY" },
    { 2, 119, "NETWORK LAYER/SETUP PROGRAM" },
    { 2, 128, "MARK DOCHTERMANN" },
    { 2, 147, "SOUND ENGINE" },
    { 2, 156, "JIM DOSE" },
    { 3, 75, "SOUND & MUSIC DEVELOPMENT" },
    { 3, 84, "ROBERT PRINCE" },
    { 3, 93, "LEE JACKSON" },
    { 3, 112, "VOICE TALENT" },
    { 3, 121, "LANI MINELLA - VOICE PRODUCER" },
    { 3, 130, "JON ST. JOHN AS \"DUKE NUKEM\"" },
    { 4, 50, "SPECIAL THANKS TO" },
    { 4, 59, "STEVE BLACKBURN, SCOTT MILLER" },
    { 4, 68, "TOM HALL, TERRY NAGY, JOE SIEGLER" },
    { 4, 87, "THE 3D REALMS BETA TESTERS" },
    { 4, 96, "MIKE BARTELT, WAYNE BENNER" },
    { 4, 105, "GLENN BRENSINGER, ROB BROWN" },
    { 4, 114, "ERIK HARRIS, KEN HECKBERT" },
    { 4, 123, "TERRY HERRIN, GREG HIVELY" },
    { 4, 132, "HANK LEUKART, KEN MAYER" },
    { 4, 141, "JEFF RAUSCH, KELLY ROGERS" },
    { 4, 150, "MIKE DUNCAN, DOUG HOWELL" },
    { 5, 50, "DUKE NUKEM IS A TRADEMARK OF" },
    { 5, 59, "3D REALMS ENTERTAINMENT" },
    { 5, 78, "DUKE NUKEM 3D" },
    { 5, 87, "(C) 1996 3D REALMS ENTERTAINMENT" },
    { 5, 106, "PLEASE READ LICENSE.DOC FOR SHAREWARE" },
    { 5, 115, "DISTRIBUTION GRANTS AND RESTRICTIONS" },
    { 5, 134, "MADE IN DALLAS, TEXAS USA" },
};
#endif

#if DUKE3D_GAMEVER >= GAMEVER_11 && DUKE3D_GAMEVER < GAMEVER_13D
// 1.1's credits: seven pages, with its own hardware page, as its menus() draws them
// (page, y, line) (G11-G37).
static const struct { short page, y; const char *text; } credits11[] =
{
    { 0, 50, "ORIGINAL CONCEPT" },
    { 0, 59, "TODD REPLOGLE" },
    { 0, 68, "ALLEN H. BLUM III" },
    { 0, 90, "PRODUCED & DIRECTED BY" },
    { 0, 99, "GREG MALONE" },
    { 0, 121, "EXECUTIVE PRODUCER" },
    { 0, 130, "GEORGE BROUSSARD" },
    { 1, 50, "GAME PROGRAMMING" },
    { 1, 59, "TODD REPLOGLE" },
    { 1, 80, "BUILD ENGINE/TOOLS/NET/MODEM" },
    { 1, 89, "KEN SILVERMAN" },
    { 1, 111, "MAP DESIGN" },
    { 1, 120, "ALLEN H BLUM III" },
    { 1, 129, "RICHARD GRAY" },
    { 2, 35, "3D MODELING" },
    { 2, 44, "CHUCK JONES" },
    { 2, 53, "SAPPHIRE CORPORATION" },
    { 2, 73, "ARTWORK" },
    { 2, 82, "DIRK JONES" },
    { 2, 91, "STEPHEN HORNBACK" },
    { 2, 100, "JAMES STOREY" },
    { 2, 119, "NETWORK LAYER/SETUP PROGRAM" },
    { 2, 128, "MARK DOCHTERMANN" },
    { 2, 147, "SOUND ENGINE" },
    { 2, 156, "JIM DOSE" },
    { 3, 75, "SOUND & MUSIC DEVELOPMENT" },
    { 3, 84, "ROBERT PRINCE" },
    { 3, 93, "LEE JACKSON" },
    { 3, 112, "VOICE TALENT" },
    { 3, 121, "LANI MINELLA - VOICE PRODUCER" },
    { 3, 130, "JON ST. JOHN AS \"DUKE NUKEM\"" },
    { 4, 48, "SPECIAL THANKS TO" },
    { 4, 57, "STEVE BLACKBURN, SCOTT MILLER" },
    { 4, 66, "TOM HALL, TERRY NAGY, JOE SIEGLER" },
    { 4, 75, "COLLEEN COMPTON, FORMGEN, INC." },
    { 4, 93, "THE 3D REALMS BETA TESTERS" },
    { 4, 102, "MIKE BARTELT, WAYNE BENNER" },
    { 4, 111, "GLENN BRENSINGER, ROB BROWN" },
    { 4, 120, "ERIK HARRIS, KEN HECKBERT" },
    { 4, 129, "TERRY HERRIN, GREG HIVELY" },
    { 4, 138, "HANK LEUKART, KEN MAYER" },
    { 4, 147, "JEFF RAUSCH, KELLY ROGERS" },
    { 4, 156, "MIKE DUNCAN, DOUG HOWELL" },
    { 5, 50, "COMPANY PRODUCT SUPPORT" },
    { 5, 68, "THE FOLLOWING COMPANIES WERE COOL" },
    { 5, 77, "ENOUGH TO GIVE US LOTS OF STUFF" },
    { 5, 86, "DURING THE MAKING OF DUKE NUKEM 3D." },
    { 5, 104, "ALTEC LANSING MULTIMEDIA" },
    { 5, 113, "CREATIVE LABS, INC." },
    { 5, 131, "THANKS FOR THE HARDWARE, GUYS." },
    { 6, 50, "DUKE NUKEM IS A TRADEMARK OF" },
    { 6, 59, "3D REALMS ENTERTAINMENT" },
    { 6, 78, "DUKE NUKEM 3D" },
    { 6, 87, "(C) 1996 3D REALMS ENTERTAINMENT" },
    { 6, 106, "PLEASE READ LICENSE.DOC FOR SHAREWARE" },
    { 6, 115, "DISTRIBUTION GRANTS AND RESTRICTIONS" },
    { 6, 134, "MADE IN DALLAS, TEXAS USA" },
};
#endif

#if DUKE3D_GAMEVER >= GAMEVER_13D && DUKE3D_GAMEVER < GAMEVER_14
// 1.3D's credits: eight pages of text, as its menus() draws them (page, y, line).
static const struct { short page, y; const char *text; } credits13[] =
{
    { 0, 40, "ORIGINAL CONCEPT" },
    { 0, 49, "TODD REPLOGLE" },
    { 0, 58, "ALLEN H. BLUM III" },
    { 0, 76, "PRODUCED & DIRECTED BY" },
    { 0, 85, "GREG MALONE" },
    { 0, 103, "EXECUTIVE PRODUCER" },
    { 0, 112, "GEORGE BROUSSARD" },
    { 0, 130, "BUILD ENGINE" },
    { 0, 139, "KEN SILVERMAN" },
    { 1, 40, "GAME PROGRAMMING" },
    { 1, 49, "TODD REPLOGLE" },
    { 1, 67, "3D ENGINE/TOOLS/NET" },
    { 1, 76, "KEN SILVERMAN" },
    { 1, 94, "NETWORK LAYER/SETUP PROGRAM" },
    { 1, 103, "MARK DOCHTERMANN" },
    { 2, 40, "MAP DESIGN" },
    { 2, 49, "ALLEN H BLUM III" },
    { 2, 58, "RICHARD GRAY" },
    { 2, 76, "3D MODELING" },
    { 2, 85, "CHUCK JONES" },
    { 2, 94, "SAPPHIRE CORPORATION" },
    { 2, 112, "ARTWORK" },
    { 2, 121, "DIRK JONES, STEPHEN HORNBACK" },
    { 2, 130, "JAMES STOREY, DAVID DEMARET" },
    { 2, 139, "DOUGLAS R WOOD" },
    { 3, 40, "SOUND ENGINE" },
    { 3, 49, "JIM DOSE" },
    { 3, 67, "SOUND & MUSIC DEVELOPMENT" },
    { 3, 76, "ROBERT PRINCE" },
    { 3, 85, "LEE JACKSON" },
    { 3, 103, "VOICE TALENT" },
    { 3, 112, "LANI MINELLA - VOICE PRODUCER" },
    { 3, 121, "JON ST. JOHN AS \"DUKE NUKEM\"" },
    { 4, 60, "GRAPHIC DESIGN" },
    { 4, 69, "PACKAGING, MANUAL, ADS" },
    { 4, 78, "ROBERT M. ATKINS" },
    { 4, 87, "MICHAEL HADWIN" },
    { 4, 105, "SPECIAL THANKS TO" },
    { 4, 114, "STEVEN BLACKBURN, TOM HALL" },
    { 4, 123, "SCOTT MILLER, JOE SIEGLER" },
    { 4, 132, "TERRY NAGY, COLLEEN COMPTON" },
    { 4, 141, "HASH INC., FORMGEN, INC." },
    { 5, 49, "THE 3D REALMS BETA TESTERS" },
    { 5, 67, "NATHAN ANDERSON, WAYNE BENNER" },
    { 5, 76, "GLENN BRENSINGER, ROB BROWN" },
    { 5, 85, "ERIK HARRIS, KEN HECKBERT" },
    { 5, 94, "TERRY HERRIN, GREG HIVELY" },
    { 5, 103, "HANK LEUKART, ERIC BAKER" },
    { 5, 112, "JEFF RAUSCH, KELLY ROGERS" },
    { 5, 121, "MIKE DUNCAN, DOUG HOWELL" },
    { 5, 130, "BILL BLAIR" },
    { 6, 32, "COMPANY PRODUCT SUPPORT" },
    { 6, 50, "THE FOLLOWING COMPANIES WERE COOL" },
    { 6, 59, "ENOUGH TO GIVE US LOTS OF STUFF" },
    { 6, 68, "DURING THE MAKING OF DUKE NUKEM 3D." },
    { 6, 86, "ALTEC LANSING MULTIMEDIA" },
    { 6, 95, "FOR TONS OF SPEAKERS AND THE" },
    { 6, 104, "THX-LICENSED SOUND SYSTEM" },
    { 6, 113, "FOR INFO CALL 1-800-548-0620" },
    { 6, 131, "CREATIVE LABS, INC." },
    { 6, 149, "THANKS FOR THE HARDWARE, GUYS." },
    { 7, 50, "DUKE NUKEM IS A TRADEMARK OF" },
    { 7, 59, "3D REALMS ENTERTAINMENT" },
    { 7, 78, "DUKE NUKEM 3D" },
    { 7, 87, "(C) 1996 3D REALMS ENTERTAINMENT" },
    { 7, 115, "MADE IN DALLAS, TEXAS USA" },
};
#endif

#if DUKE3D_GAMEVER < GAMEVER_10
#define MAIN_MENU_TOP 55
#define GAME_MENU_TOP 55
#else
#define MAIN_MENU_TOP 67-3
#define GAME_MENU_TOP 67
#endif

/*
================
menus

Service menu input and packets, then draw and operate the current page.
The page number also carries confirmation and save-name entry states.
================
*/
void menus(void)
{
    int count;
    short shorttmp = 0;
    short c,x;
    volatile int32_t l;
// CTW - REMOVED
//  int tenerr;
// CTW END - REMOVED

    _handle_events();
    getpackets();

    if(ControllerType == 1 && CONTROL_MousePresent)
    {
        if(buttonstat != 0 && !onbar)
        {
            x = MOUSE_GetButtons()<<3;
            if( x ) buttonstat = x<<3;
            else buttonstat = 0;
        }
        else
            buttonstat = MOUSE_GetButtons();
    }
    else buttonstat = 0;

    /* Release the save-preview cache lock when the menu is no longer active. */
    if( (ps[myconnectindex].gm&MODE_MENU) == 0 )
    {
        walock[MAXTILES-3] = 1;
        return;
    }

    ps[myconnectindex].gm &= (0xff-MODE_TYPE);
    ps[myconnectindex].fta = 0;

    x = 0;

    /* Pulse the highlighted row from the Build clock, independently of input. */
    sh = 4-(sintable[(totalclock<<4)&2047]>>11);

    if(!(current_menu >= 1000 && current_menu <= 2999 && current_menu >= 300 && current_menu <= 369))
        vscrn();

    // icculus.org-added menus?
    /* Port-added configuration pages occupy a separate range of page numbers. */
    if (current_menu >= 30000)
        non3dr_menus();
    else switch(current_menu)
    {
        case 25000:
            gametext(160,90,"SELECT A SAVE SPOT BEFORE",0,2+8+16);
            gametext(160,90+9,"YOU QUICK RESTORE.",0,2+8+16);

            x = probe(186,124,0,0);
            if(x >= -1)
            {
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
                ps[myconnectindex].gm &= ~MODE_MENU;
            }
            break;

        case 20000:
            x = probe(326,190,0,0);
#if DUKE3D_GAMEVER < GAMEVER_10
            gametext(160,10,"YOU ARE PLAYING THE SHAREWARE",0,2+8+16);
            gametext(160,19,"VERSION OF DUKE NUKEM 3D.  THIS",0,2+8+16);
            gametext(160,28,"VERSION CONTAINS EPISODE 1 OF",0,2+8+16);
            gametext(160,37,"THE 3-EPISODE COMPLETE",0,2+8+16);
            gametext(160,46,"REGISTERED VERSION",0,2+8+16);
            gametext(28,64,"* 18 ADDITIONAL LEVELS",0,2+8+16);
            gametext(28,73,"* SEVERAL NEW MONSTERS",0,2+8+16);
            gametext(28,82,"* 3 ADDITIONAL SUPER-COOL WEAPONS",0,2+8+16);
            gametext(28,91,"* UP TO 8 SIMULTANEOUS",0,2+8+16);
            gametext(28,100,"    NETWORK PLAYERS",0,2+8+16);
            gametext(28,109,"* LOTS MORE WILD SUPRISES",0,2+8+16);
            gametext(28,118,"    AND HOT ACTION!",0,2+8+16);
            gametext(160,134,"IF YOU WISH TO UPGRADE TO THE",0,2+8+16);
            gametext(160,143,"REGISTERED VERSION OF DUKE NUKEM 3D,",0,2+8+16);
            gametext(160,152,"CALL US AT 1-800-GAME123 OR SEE YOUR,",0,2+8+16);
            gametext(160,161,"LOCAL SOFTWARE DEALER.",0,2+8+16);
#else
            gametext(160,50-8,"YOU ARE PLAYING THE SHAREWARE",0,2+8+16);
            gametext(160,59-8,"VERSION OF DUKE NUKEM 3D.  WHILE",0,2+8+16);
            gametext(160,68-8,"THIS VERSION IS REALLY COOL, YOU",0,2+8+16);
            gametext(160,77-8,"ARE MISSING OVER 75% OF THE TOTAL",0,2+8+16);
            gametext(160,86-8,"GAME, ALONG WITH OTHER GREAT EXTRAS",0,2+8+16);
            gametext(160,95-8,"AND GAMES, WHICH YOU'LL GET WHEN",0,2+8+16);
            gametext(160,104-8,"YOU ORDER THE COMPLETE VERSION AND",0,2+8+16);
            gametext(160,113-8,"GET THE FINAL TWO EPISODES.",0,2+8+16);

            gametext(160,113+8,"PLEASE READ THE 'HOW TO ORDER' ITEM",0,2+8+16);
            gametext(160,122+8,"ON THE MAIN MENU IF YOU WISH TO",0,2+8+16);
            gametext(160,131+8,"UPGRADE TO THE FULL REGISTERED",0,2+8+16);
            gametext(160,140+8,"VERSION OF DUKE NUKEM 3D.",0,2+8+16);
#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 and 1.0 have no "press any key" line (G11-G37)
            gametext(160,149+16,"PRESS ANY KEY...",0,2+8+16);
#endif

#endif
            if( x >= -1 ) cmenu(100);
            break;
// CTW - REMOVED
/*      case 20001:
            x = probe(188,80+32+32,0,0);
            gametext(160,86-8,"You must be in Windows 95 to",0,2+8+16);
            gametext(160,86,"play on TEN",0,2+8+16);
            gametext(160,86+32,"PRESS ANY KEY...",0,2+8+16);
            if(x >= -1) cmenu(0);
            break;

        case 20002:
            x = probe(188,80+32+32+32,0,0);
            gametext(160,86-8,"MISSING FILE: TENGAME.INI.  PLEASE",0,2+8+16);
            gametext(160,86,"CONNECT TO TEN BY LAUNCHING THE",0,2+8+16);
            gametext(160,86+8,"CONNECT TO TEN SHORTCUT OR CONTACT",0,2+8+16);
            gametext(160,86+8+8,"CUSTOMER SUPPORT AT 1-800-8040-TEN.",0,2+8+16);
            gametext(160,86+8+8+32,"PRESS ANY KEY...",0,2+8+16);
            if(x >= -1) cmenu(0);
            break;
        case 20003:
            x = probe(188,80+32+32,0,0);
            gametext(160,86-8,"BAD TEN INSTALL:  PLEASE RE-INSTALL",0,2+8+16);
            gametext(160,86,"BAD TEN INSTALL:  PLEASE RE-INSTALL TEN",0,2+8+16);
            gametext(160,86+32,"PRESS ANY KEY...",0,2+8+16);
            if(x >= -1) cmenu(0);
            break;
        case 20005:
            x = probe(188,80+32+32,0,0);
            gametext(160,86-8,"GET THE LATEST TEN SOFTWARE AT",0,2+8+16);
            gametext(160,86,"HTTP://WWW.TEN.NET",0,2+8+16);
            gametext(160,86+32,"PRESS ANY KEY...",0,2+8+16);
            if(x >= -1) cmenu(0);
            break;*/
// CTW END - REMOVED

        case 15001:
        case 15000:

            gametext(160,90,"LOAD last game:",0,2+8+16);

            sprintf(tempbuf,"\"%s\"",ud.savegame[lastsavedpos]);
            gametext(160,99,tempbuf,0,2+8+16);

            gametext(160,99+9,"(Y/N)",0,2+8+16);

            if( KB_KeyPressed(sc_Escape) || KB_KeyPressed(sc_N) || RMB)
            {
#if DUKE3D_GAMEVER < GAMEVER_10
                if(current_menu == 15000 && KB_KeyPressed(sc_N))
#else
                if(sprite[ps[myconnectindex].i].extra <= 0)
#endif
                {
                    enterlevel(MODE_GAME);
                    return;
                }

                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);

                ps[myconnectindex].gm &= ~MODE_MENU;
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            if(  KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                FX_StopAllSounds();

                if(ud.multimode > 1)
                {
                    loadplayer(-1-lastsavedpos);
                    ps[myconnectindex].gm = MODE_GAME;
                }
                else
                {
                    c = loadplayer(lastsavedpos);
                    if(c == 0)
                        ps[myconnectindex].gm = MODE_GAME;
                }
            }

            probe(186,124+9,0,0);

            break;

        case 10000:
        case 10001:

            c = 60;
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,
#if DUKE3D_GAMEVER < GAMEVER_10
                "PARENTAL LOCK"
#else
                "ADULT MODE"
#endif
);

            x = probe(60,50+16,16,2);
            if(x == -1) { cmenu(200); break; }

            menutext(c,50+16,SHX(-2),PHX(-2),
#if DUKE3D_GAMEVER < GAMEVER_10
                "PARENTAL LOCK"
#else
                "ADULT MODE"
#endif
);
            menutext(c,50+16+16,SHX(-3),PHX(-3),"ENTER PASSWORD");

            if(ud.lockout) menutext(c+160+40,50+16,0,0,"OFF");
            else menutext(c+160+40,50+16,0,0,"ON");

            if(current_menu == 10001)
            {
                gametext(160,50+16+16+16+16-12,"ENTER PASSWORD",0,2+8+16);
                x = strget((320>>1),50+16+16+16+16,buf,19, 998);

                if( x )
                {
                    if(ud.pwlockout[0] == 0 || ud.lockout == 0 )
                        strcpy(&ud.pwlockout[0],buf);
                    else if( strcmp(buf,&ud.pwlockout[0]) == 0 )
                    {
                        ud.lockout = 0;
                        buf[0] = 0;

                        for(x=0;x<numanimwalls;x++)
#if DUKE3D_GAMEVER >= GAMEVER_10
                            if( wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2 )
#endif
                                    if( wall[animwall[x].wallnum].extra >= 0 )
                                        wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;

                    }
                    current_menu = 10000;
                    KB_ClearKeyDown(sc_Enter);
                    KB_ClearKeyDown(sc_kpad_Enter);
                    KB_FlushKeyboardQueue();
                }
            }
            else
            {
                if(x == 0)
                {
                    if( ud.lockout == 1 )
                    {
                        if(ud.pwlockout[0] == 0)
                        {
                            ud.lockout = 0;
#if DUKE3D_GAMEVER >= GAMEVER_10
                            for(x=0;x<numanimwalls;x++)
#if DUKE3D_GAMEVER >= GAMEVER_10
                            if( wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2 )
#endif
                                    if( wall[animwall[x].wallnum].extra >= 0 )
                                        wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
#endif
                        }
                        else
                        {
                            buf[0] = 0;
                            current_menu = 10001;
                            inputloc = 0;
                            KB_FlushKeyboardQueue();
                        }
                    }
                    else
                    {
                        ud.lockout = 1;

                        for(x=0;x<numanimwalls;x++)
                            switch(wall[animwall[x].wallnum].picnum)
                            {
                                case FEMPIC1:
#if DUKE3D_GAMEVER >= GAMEVER_10
                                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                                    break;
#endif
                                case FEMPIC2:
                                case FEMPIC3:
#if DUKE3D_GAMEVER < GAMEVER_10
                                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
#else
                                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
#endif
                                    break;
                            }
                    }
                }

                else if(x == 1)
                {
                    current_menu = 10001;
                    inputloc = 0;
                    KB_FlushKeyboardQueue();
                }
            }

            break;

        case 1000:
        case 1001:
        case 1002:
        case 1003:
        case 1004:
        case 1005:
        case 1006:
        case 1007:
        case 1008:
        case 1009:

            if (widescreen) clearallviews(0L);
            rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"LOAD GAME");
            rotatesprite(101<<16,97<<16,65536,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);

            dispnames();

#if DUKE3D_GAMEVER >= GAMEVER_10
            sprintf(tempbuf,"PLAYERS: %-2d                      ",numplr);
            gametext(160,158,tempbuf,0,2+8+16);
#endif

#if DUKE3D_GAMEVER < GAMEVER_10
            sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+volnum,1+levnum,plrskl+1);
#else
            sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+volnum,1+levnum,plrskl);
#endif
            gametext(160,170,tempbuf,0,2+8+16);

            gametext(160,90,"LOAD game:",0,2+8+16);
            sprintf(tempbuf,"\"%s\"",ud.savegame[current_menu-1000]);
            gametext(160,99,tempbuf,0,2+8+16);
            gametext(160,99+9,"(Y/N)",0,2+8+16);

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                lastsavedpos = current_menu-1000;

                KB_FlushKeyboardQueue();
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }

                if(ud.multimode > 1)
                {
                    if( ps[myconnectindex].gm&MODE_GAME )
                    {
                        loadplayer(-1-lastsavedpos);
                        ps[myconnectindex].gm = MODE_GAME;
                    }
                    else
                    {
                        tempbuf[0] = 126;
                        tempbuf[1] = lastsavedpos;
                        for(x=connecthead;x>=0;x=connectpoint2[x])
                            if(x != myconnectindex)
                                sendpacket(x,tempbuf,2);

                        getpackets();

                        loadplayer(lastsavedpos);

                        multiflag = 0;
                    }
                }
                else
                {
                    c = loadplayer(lastsavedpos);
                    if(c == 0)
                        ps[myconnectindex].gm = MODE_GAME;
                }

                break;
            }
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                sound(EXITMENUSOUND);
                if(ps[myconnectindex].gm&MODE_DEMO) cmenu(300);
                else
                {
                    ps[myconnectindex].gm &= ~MODE_MENU;
                    if(ud.multimode < 2 && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                }
            }

            probe(186,124+9,0,0);

            break;

        case 1500:

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                cmenu(100);
            }
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
                ps[myconnectindex].gm &= ~MODE_MENU;
                sound(EXITMENUSOUND);
                break;
            }
            probe(186,124,0,0);
            gametext(160,90,"ABORT this game?",0,2+8+16);
            gametext(160,90+9,"(Y/N)",0,2+8+16);

            break;

        case 2000:
        case 2001:
        case 2002:
        case 2003:
        case 2004:
        case 2005:
        case 2006:
        case 2007:
        case 2008:
        case 2009:

            if (widescreen) clearallviews(0L);
            rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SAVE GAME");

            rotatesprite(101<<16,97<<16,65536L,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
#if DUKE3D_GAMEVER >= GAMEVER_10
            sprintf(tempbuf,"PLAYERS: %-2d                      ",ud.multimode);
            gametext(160,158,tempbuf,0,2+8+16);
#endif

#if DUKE3D_GAMEVER < GAMEVER_10
            sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill+1);
#else
            sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
#endif
            gametext(160,170,tempbuf,0,2+8+16);

            dispnames();

            gametext(160,90,"OVERWRITE previous SAVED game?",0,2+8+16);
            gametext(160,90+9,"(Y/N)",0,2+8+16);

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                inputloc = strlen(&ud.savegame[current_menu-2000][0]);

                cmenu(current_menu-2000+360);

                KB_FlushKeyboardQueue();
                break;
            }
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                cmenu(351);
                sound(EXITMENUSOUND);
            }

            probe(186,124,0,0);

            break;

        case 990:
        case 991:
        case 992:
        case 993:
        case 994:
        case 995:
        case 996:
        case 997:
#if DUKE3D_GAMEVER < GAMEVER_10
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"CREDITS");
            x=probe(190,184,0,0);
            if(x == -1) { cmenu(0); break; }
            if(x >= 0 || KB_KeyPressed(sc_Space))
            {
                KB_ClearKeyDown(sc_Space);
                cmenu(current_menu == 996 ? 0 : current_menu+1);
            }
            for(l=0;l<(int)(sizeof(credits099)/sizeof(credits099[0]));l++)
                if(credits099[l].page == current_menu-990)
                    gametext(160,credits099[l].y,(char *)credits099[l].text,0,2+8+16);
            break;
#else


//            rotatesprite(c<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
//            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
//            menutext(c,24,0,0,"CREDITS");
#if DUKE3D_GAMEVER < GAMEVER_14
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"CREDITS");
#endif

            if(KB_KeyPressed(sc_Escape)) { cmenu(0); break; }

            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
#if DUKE3D_GAMEVER < GAMEVER_11
                if(current_menu < 990) current_menu = 995;
#elif DUKE3D_GAMEVER < GAMEVER_13D
                if(current_menu < 990) current_menu = 996;
#elif DUKE3D_GAMEVER < GAMEVER_14
                if(current_menu < 990) current_menu = 997;
#else
                if(current_menu < 990) current_menu = 992;
#endif
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
                sound(KICK_HIT);
                current_menu++;
#if DUKE3D_GAMEVER < GAMEVER_11
                if(current_menu > 995) current_menu = 990;
#elif DUKE3D_GAMEVER < GAMEVER_13D
                if(current_menu > 996) current_menu = 990;
#elif DUKE3D_GAMEVER < GAMEVER_14
                if(current_menu > 997) current_menu = 990;
#else
                if(current_menu > 992) current_menu = 990;
#endif
            }

#if DUKE3D_GAMEVER < GAMEVER_11
            for(l=0;l<(int)(sizeof(credits10)/sizeof(credits10[0]));l++)
                if(credits10[l].page == current_menu-990)
                    gametext(160,credits10[l].y,(char *)credits10[l].text,0,2+8+16);
#elif DUKE3D_GAMEVER < GAMEVER_13D
            for(l=0;l<(int)(sizeof(credits11)/sizeof(credits11[0]));l++)
                if(credits11[l].page == current_menu-990)
                    gametext(160,credits11[l].y,(char *)credits11[l].text,0,2+8+16);
#elif DUKE3D_GAMEVER < GAMEVER_14
            for(l=0;l<(int)(sizeof(credits13)/sizeof(credits13[0]));l++)
                if(credits13[l].page == current_menu-990)
                    gametext(160,credits13[l].y,(char *)credits13[l].text,0,2+8+16);
#else
            switch(current_menu)
            {
                case 990:
                case 991:
                case 992:
                   rotatesprite(160<<16,200<<15,65536L,0,2504+current_menu-990,0,0,10+64,0,0,xdim-1,ydim-1);

                   break;

            }
#endif
            break;

#endif

        case 0:
            c = (320>>1);
            rotatesprite(c<<16,(28<<16)-5,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
#if DUKE3D_GAMEVER >= GAMEVER_14 // no Plutonium Pak logo in 1.3D's menus
            rotatesprite((c+100)<<16,(36<<16)-5,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
#endif
// CTW - MODIFICATION
//          x = probe(c,MAIN_MENU_TOP,16,7);
            count = 6;
#if NET_MENU && DUKE3D_GAMEVER >= GAMEVER_10
            count++;
#endif
            x = probe(c,MAIN_MENU_TOP,16,count);
// CTW END - MODIFICATION
            if(x >= 0)
            {
                if( ud.multimode > 1 && x == 0 && ud.recstat != 2)
                {
                    if( movesperpacket == 4 && myconnectindex != connecthead )
                        break;

                    last_zero = 0;
                    cmenu( 600 );
                }
                else
                {
                    last_zero = x;
                    switch(x)
                    {
                        case 0:
                            cmenu(100);
                            break;
// CTW - MODIFICATION
// Shifted the entire menu input results up one.
/*                      case 1:
                            if(movesperpacket == 4 || numplayers > 1)
                                break;

                            tenBnSetExitRtn(dummyfunc);
                            setDebugMsgRoutine(dummymess);
                            tenerr = tenBnStart();

                            switch(tenerr)
                            {
                                case eTenBnNotInWindows:
                                    cmenu(20001);
                                    break;
                                case eTenBnBadGameIni:
                                    cmenu(20002);
                                    break;
                                case eTenBnBadTenIni:
                                    cmenu(20003);
                                    break;
                                case eTenBnBrowseCancel:
                                    cmenu(20004);
                                    break;
                                case eTenBnBadTenInst:
                                    cmenu(20005);
                                    break;
                                default:
                                    playonten = 1;
                                    gameexit(" ");
                                    break;
                            }
                            break;*/
                        case 1: cmenu(200);break;
                        case 2:
                            if(movesperpacket == 4 && connecthead != myconnectindex)
                                break;
                            cmenu(300);
                            break;
                        case 3: KB_FlushKeyboardQueue();cmenu(400);break;
                        case 4: cmenu(990);break;
#if NET_MENU && DUKE3D_GAMEVER >= GAMEVER_10
                        case 5: cmenu(30000);break;
                        case 6: cmenu(500);break;
#else
                        case 5: cmenu(500);break;
#endif

// CTW END - MODIFICATION
                    }
                }
            }

            if(KB_KeyPressed(sc_Q)) cmenu(500);

            if(x == -1)
            {
                ps[myconnectindex].gm &= ~MODE_MENU;
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            if(movesperpacket == 4)
            {
                if( myconnectindex == connecthead )
                    menutext(c,MAIN_MENU_TOP,SHX(-2),PHX(-2),"NEW GAME");
                else
                    menutext(c,MAIN_MENU_TOP,SHX(-2),1,"NEW GAME");
            }
            else
                menutext(c,MAIN_MENU_TOP,SHX(-2),PHX(-2),"NEW GAME");

// CTW - REMOVED
/*          if(movesperpacket != 4 && numplayers < 2)
                menutext(c,MAIN_MENU_TOP+16,SHX(-3),PHX(-3),"PLAY ON TEN");
            else
                menutext(c,MAIN_MENU_TOP+16,SHX(-3),1,"PLAY ON TEN");*/
// CTW END - REMOVED

// CTW - MODIFICATION - Not going to comment out the exact old code here.
// I shifted up every menu option by 16.
// I shifted up every menu result by 1.
            menutext(c,MAIN_MENU_TOP+16,SHX(-3),PHX(-3),"OPTIONS");

            if(movesperpacket == 4 && connecthead != myconnectindex)
                menutext(c,MAIN_MENU_TOP+16+16,SHX(-4),1,"LOAD GAME");
            else menutext(c,MAIN_MENU_TOP+16+16,SHX(-4),PHX(-4),"LOAD GAME");

#ifndef VOLUMEALL
            menutext(c,MAIN_MENU_TOP+16+16+16,SHX(-5),PHX(-5),"HOW TO ORDER");
#else
            menutext(c,MAIN_MENU_TOP+16+16+16,SHX(-5),PHX(-5),"HELP");
#endif
            menutext(c,MAIN_MENU_TOP+16+16+16+16,SHX(-6),PHX(-6),"CREDITS");

#if NET_MENU && DUKE3D_GAMEVER >= GAMEVER_10
            menutext(c,MAIN_MENU_TOP+16+16+16+16+16,SHX(-7),PHX(-7),"NETWORK GAME");
            menutext(c,MAIN_MENU_TOP+16+16+16+16+16+16,SHX(-8),PHX(-8),"QUIT");
#else
            menutext(c,MAIN_MENU_TOP+16+16+16+16+16,SHX(-7),PHX(-7),"QUIT");
#endif

            break;
// CTW END - MODIFICATION

        case 50:
            c = (320>>1);
            rotatesprite(c<<16,32<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
#if DUKE3D_GAMEVER >= GAMEVER_14 // no Plutonium Pak logo in 1.3D's menus
            rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
#endif
            x = probe(c,GAME_MENU_TOP,16,7);
            switch(x)
            {
                case 0:
                    if(movesperpacket == 4 && myconnectindex != connecthead)
                        break;
                    if(ud.multimode < 2 || ud.recstat == 2)
                        cmenu(1500);
                    else
                    {
                        cmenu(600);
                        last_fifty = 0;
                    }
                    break;
                case 1:
                    if(movesperpacket == 4 && connecthead != myconnectindex)
                        break;
                    if(ud.recstat != 2)
                    {
                        last_fifty = 1;
                        cmenu(350);
                        setview(0,0,xdim-1,ydim-1);
                    }
                    break;
                case 2:
                    if(movesperpacket == 4 && connecthead != myconnectindex)
                        break;
                    last_fifty = 2;
                    cmenu(300);
                    break;
                case 3:
                    last_fifty = 3;
                    cmenu(200);
                    break;
                case 4:
                    last_fifty = 4;
                    KB_FlushKeyboardQueue();
                    cmenu(400);
                    break;
                case 5:
                    if(numplayers < 2)
                    {
                        last_fifty = 5;
                        cmenu(501);
                    }
                    break;
                case 6:
                    last_fifty = 6;
                    cmenu(500);
                    break;
                case -1:
                    ps[myconnectindex].gm &= ~MODE_MENU;
                    if(ud.multimode < 2 && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                    break;
            }

            if( KB_KeyPressed(sc_Q) )
                cmenu(500);

            if(movesperpacket == 4 && connecthead != myconnectindex)
            {
                menutext(c,GAME_MENU_TOP                  ,SHX(-2),1,"NEW GAME");
                menutext(c,GAME_MENU_TOP+16               ,SHX(-3),1,"SAVE GAME");
                menutext(c,GAME_MENU_TOP+16+16            ,SHX(-4),1,"LOAD GAME");
            }
            else
            {
                menutext(c,GAME_MENU_TOP                  ,SHX(-2),PHX(-2),"NEW GAME");
                menutext(c,GAME_MENU_TOP+16               ,SHX(-3),PHX(-3),"SAVE GAME");
                menutext(c,GAME_MENU_TOP+16+16            ,SHX(-4),PHX(-4),"LOAD GAME");
            }

            menutext(c,GAME_MENU_TOP+16+16+16         ,SHX(-5),PHX(-5),"OPTIONS");
#ifndef VOLUMEALL
            menutext(c,GAME_MENU_TOP+16+16+16+16      ,SHX(-6),PHX(-6),"HOW TO ORDER");
#else
            menutext(c,GAME_MENU_TOP+16+16+16+16      ,SHX(-6),PHX(-6)," HELP");
#endif
            if(numplayers > 1)
                menutext(c,GAME_MENU_TOP+16+16+16+16+16   ,SHX(-7),1,"QUIT TO TITLE");
            else menutext(c,GAME_MENU_TOP+16+16+16+16+16   ,SHX(-7),PHX(-7),"QUIT TO TITLE");
            menutext(c,GAME_MENU_TOP+16+16+16+16+16+16,
                SHX(-8),PHX(-8),"QUIT GAME");
            break;

        case 100:
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SELECT AN EPISODE");
#if DUKE3D_GAMEVER < GAMEVER_10
            x = probe(160,60,20,3);
#elif defined PLUTOPAK
            if(boardfilename[0])
                x = probe(160,60,20,5);
            else x = probe(160,60,20,4);
#else
            if(boardfilename[0])
                x = probe(160,60,20,4);
            else x = probe(160,60,20,3);
#endif
            if(x >= 0)
            {
#ifdef VOLUMEONE
                if(x > 0)
                    cmenu(20000);
                else
                {
                    ud.m_volume_number = x;
                    ud.m_level_number = 0;
                    cmenu(110);
                }
#endif

#ifndef VOLUMEONE
#ifndef PLUTOPAK

                if(x == 3 && boardfilename[0])
                {
                    ud.m_volume_number = 0;
                    ud.m_level_number = 7;
                }
#else
                if(x == 4 && boardfilename[0])
                {
                    ud.m_volume_number = 0;
                    ud.m_level_number = 7;
                }
#endif

                else
                {
                    ud.m_volume_number = x;
                    ud.m_level_number = 0;
                }
                cmenu(110);
#endif
            }
            else if(x == -1)
            {
                if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);
                else cmenu(0);
            }

            menutext(160,60,SHX(-2),PHX(-2),volume_names[0]);

            c = 80;
#ifdef VOLUMEONE
            menutext(160,60+20,SHX(-3),1,volume_names[1]);
            menutext(160,60+20+20,SHX(-4),1,volume_names[2]);
#ifdef PLUTOPAK
            menutext(160,60+20+20,SHX(-5),1,volume_names[3]);
#endif
#else
            menutext(160,60+20,SHX(-3),PHX(-3),volume_names[1]);
            menutext(160,60+20+20,SHX(-4),PHX(-4),volume_names[2]);
#ifdef PLUTOPAK
            menutext(160,60+20+20+20,SHX(-5),PHX(-5),volume_names[3]);
            if(boardfilename[0])
            {

                menutext(160,60+20+20+20+20,SHX(-6),PHX(-6),"USER MAP");
                gametextpal(160,60+20+20+20+20+3,boardfilename,16+(sintable[(totalclock<<4)&2047]>>11),2);
            }
#else
            if(boardfilename[0])
            {
                menutext(160,60+20+20+20,SHX(-6),PHX(-6),"USER MAP");
                gametext(160,60+20+20+20+6,boardfilename,2,2+8+16);
            }
#endif

#endif
            break;

        case 110:
            c = (320>>1);
            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(c,24,0,0,"SELECT SKILL");
#if DUKE3D_GAMEVER < GAMEVER_10
            x=probe(c,50,16,5);
            if(x >= 0)
            {
                ud.m_player_skill=x;
                if(x == 4) ud.m_respawn_monsters=1;
                else ud.respawn_monsters=0;
                newgame(ud.m_volume_number,ud.m_level_number,x);
                enterlevel(MODE_GAME);
            }
            else if(x == -1) cmenu(100);
            for(l=0;l<5;l++) menutext(c,50+16*l,SHX(-l-2),PHX(-l-2),skill_names[l]);
            break;
#else
            x = probe(c,70,19,4);
            if(x >= 0)
            {
                switch(x)
                {
                    case 0: globalskillsound = JIBBED_ACTOR6;break;
                    case 1: globalskillsound = BONUS_SPEECH1;break;
                    case 2: globalskillsound = DUKE_GETWEAPON2;break;
                    case 3: globalskillsound = JIBBED_ACTOR5;break;
                }

                sound(globalskillsound);

                ud.m_player_skill = x+1;
                if(x == 3) ud.m_respawn_monsters = 1;
                else ud.m_respawn_monsters = 0;

                ud.m_monsters_off = ud.monsters_off = 0;

                ud.m_respawn_items = 0;
                ud.m_respawn_inventory = 0;

                ud.multimode = 1;

                if(ud.m_volume_number == 3)
                {
                    flushperms();
                    setview(0,0,xdim-1,ydim-1);
                    clearview(0L);
                    nextpage();
                }

                newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
                enterlevel(MODE_GAME);
            }
            else if(x == -1)
            {
                cmenu(100);
                KB_FlushKeyboardQueue();
            }

            menutext(c,70,SHX(-2),PHX(-2),skill_names[0]);
            menutext(c,70+19,SHX(-3),PHX(-3),skill_names[1]);
            menutext(c,70+19+19,SHX(-4),PHX(-4),skill_names[2]);
            menutext(c,70+19+19+19,SHX(-5),PHX(-5),skill_names[3]);
            break;

#endif

        case 200:
#if DUKE3D_GAMEVER < GAMEVER_10
            {
                static char *labels[]={"DETAIL","SHADOWS","SCREEN TILTING","SCREEN SIZE",
                    "BRIGHTNESS","MOUSE SENSITIVITY","SOUNDS","PARENTAL LOCK"};
                short value;
                rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
                menutext(160,24,0,0,"OPTIONS");
                count=9;
#if CONTROLS_CONFIG_MENU
                count++;
#endif
                onbar=(probey>=3 && probey<=5);
                x=probe(40,50,16,count);
                if(x == -1) { cmenu((ps[myconnectindex].gm&MODE_GAME)?50:0); break; }
                if(!onbar) switch(x)
                {
                    case 0: ud.detail=1-ud.detail; break;
                    case 1: ud.shadows=1-ud.shadows; break;
                    case 2: ud.screen_tilting=1-ud.screen_tilting; break;
                    case 6: if(FXDevice != 13) cmenu(700); break;
                    case 7: cmenu(10000); break;
#if CONTROLS_CONFIG_MENU
                    case 8: cmenu(MENU_CONTROLS); break;
#endif
                    default:
                        if(x == count-1)
                        {
                            video_pagebegin();
                            cmenu(710);
                        }
                        break;
                }
                for(l=0;l<8;l++) menutext(40,50+16*l,SHX(-l-2),PHX(-l-2),labels[l]);
                menutext(240,50,0,0,ud.detail?"HIGH":"LOW");
                menutext(240,66,0,0,ud.shadows?"ON":"OFF");
                menutext(240,82,0,0,ud.screen_tilting?"ON":"OFF");
                value=ud.screen_size; bar(247,98,&value,-4,x==3,0,0); ud.screen_size=value;
                value=ud.brightness; bar(247,114,&value,8,x==4,0,0); ud.brightness=value;
                if(x==4) setbrightness(ud.brightness>>2,ps[myconnectindex].palette);
                value=CONTROL_GetMouseSensitivity()>>10;
                bar(247,130,&value,4,x==5,0,0); CONTROL_SetMouseSensitivity(value<<10);
#if CONTROLS_CONFIG_MENU
                menutext(40,178,SHX(-10),PHX(-10),"CONTROLS...");
#endif
                menutext(40,50+16*(count-1),SHX(-11),PHX(-11),"VIDEO...");
            }
            break;
#else


            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"OPTIONS");

            c = (320>>1)-120;

            onbar = (probey == 3 || probey == 4);
            count = 9;
#if CONTROLS_CONFIG_MENU
            count++;
#endif
            count++;  // VIDEO..., the last row
            x = probe(c+6,31,15,count);

            if(x == -1)
                { if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);else cmenu(0); }

            if(onbar == 0) switch(x)
            {
                case 0:
                    ud.detail = 1-ud.detail;
                    break;
                case 1:
                    ud.shadows = 1-ud.shadows;
                    break;
                case 2:
                    ud.screen_tilting = 1-ud.screen_tilting;
                    break;
                case 5:
                    if ( ControllerType == controltype_keyboardandmouse )
                        ud.mouseflip = 1-ud.mouseflip;
                    break;
                case 6:
                    cmenu(700);
                    break;
                case 7:
#ifndef AUSTRALIA
                    cmenu(10000);
#endif
                    break;
                case 8:
                    if( (ps[myconnectindex].gm&MODE_GAME) )
                    {
                        closedemowrite();
                        break;
                    }
                    ud.m_recstat = !ud.m_recstat;
                    break;
#if CONTROLS_CONFIG_MENU
                case 9:
                    cmenu(MENU_CONTROLS);
                    break;
#endif
                default:
                    if(x == count-1)
                    {
                        video_pagebegin();
                        cmenu(710);
                    }
                    break;
            }

            if(ud.detail) menutext(c+160+40,31,0,0,"HIGH");
            else menutext(c+160+40,31,0,0,"LOW");

            if(ud.shadows) menutext(c+160+40,31+15,0,0,"ON");
            else menutext(c+160+40,31+15,0,0,"OFF");

            switch(ud.screen_tilting)
            {
                case 0: menutext(c+160+40,31+15+15,0,0,"OFF");break;
                case 1: menutext(c+160+40,31+15+15,0,0,"ON");break;
                case 2: menutext(c+160+40,31+15+15,0,0,"FULL");break;
            }

            menutext(c,31,SHX(-2),PHX(-2),"DETAIL");
            menutext(c,31+15,SHX(-3),PHX(-3),"SHADOWS");
            menutext(c,31+15+15,SHX(-4),PHX(-4),"SCREEN TILTING");
            menutext(c,31+15+15+15,SHX(-5),PHX(-5),"SCREEN SIZE");
                shorttmp = (short) ud.screen_size;
                bar(c+167+40,31+15+15+15,(short *)&shorttmp,-4,x==3,SHX(-5),PHX(-5));
                ud.screen_size = (int32) shorttmp;

            // BRIGHTNESS is on the VIDEO page.
            if ( ControllerType == controltype_keyboardandmouse )
            {
                short sense;
                sense = CONTROL_GetMouseSensitivity()>>10;

                menutext(c,31+15+15+15+15,SHX(-7),PHX(-7),"MOUSE SENSITIVITY");
                bar(c+167+40,31+15+15+15+15,&sense,4,x==4,SHX(-7),PHX(-7));
                CONTROL_SetMouseSensitivity( sense<<10 );
                menutext(c,31+15+15+15+15+15,SHX(-7),PHX(-7),"MOUSE AIMING FLIP");

                if(ud.mouseflip) menutext(c+160+40,31+15+15+15+15+15,SHX(-7),PHX(-7),"ON");
                else menutext(c+160+40,31+15+15+15+15+15,SHX(-7),PHX(-7),"OFF");

            }
            else
            {
                short sense;
                sense = CONTROL_GetMouseSensitivity()>>10;

                menutext(c,31+15+15+15+15,SHX(-7),1,"MOUSE SENSITIVITY");
                bar(c+167+40,31+15+15+15+15,&sense,4,x==99,SHX(-7),1);
                menutext(c,31+15+15+15+15+15,SHX(-7),1,"MOUSE AIMING FLIP");

                if(ud.mouseflip) menutext(c+160+40,31+15+15+15+15+15,SHX(-7),1,"ON");
                else menutext(c+160+40,31+15+15+15+15+15,SHX(-7),1,"OFF");
            }

            menutext(c,31+15+15+15+15+15+15,SHX(-8),PHX(-8),"SOUNDS");
#ifndef AUSTRALIA
            menutext(c,31+15+15+15+15+15+15+15,SHX(-9),PHX(-9),"PARENTAL LOCK");
#else
            menutext(c,31+15+15+15+15+15+15+15,SHX(-9),1,"PARENTAL LOCK");
#endif
            if( (ps[myconnectindex].gm&MODE_GAME) && ud.m_recstat != 1 )
            {
                menutext(c,31+15+15+15+15+15+15+15+15,SHX(-10),1,"RECORD");
                menutext(c+160+40,31+15+15+15+15+15+15+15+15,SHX(-10),1,"OFF");
            }
            else
            {
                menutext(c,31+15+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"RECORD");

                if(ud.m_recstat == 1)
                    menutext(c+160+40,31+15+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"ON");
                else menutext(c+160+40,31+15+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"OFF");
            }

#if CONTROLS_CONFIG_MENU
            menutext(c,31+15+15+15+15+15+15+15+15+15,SHX(-11),PHX(-11),"CONTROLS...");
#endif
            menutext(c,31+15*(count-1),SHX(-12),PHX(-12),"VIDEO...");
            break;

#endif

        case 710:
            // VIDEO: every live video setting, rows from y 31, 15 apart, as
            // OPTIONS. A change fades out, is made at black and fades back in
            // (game.c video_request*), so the rows that ask for one only
            // take a new one once the last is made, and only on a real
            // change; RESOLUTION's arrows only pick a size, Enter applies it.
            // A row changed is saved (video_pagetouch), the others aren't.
            {
                static const char *const renderernames[4] = { "CLASSIC", "POLY SOLID", "POLY SOFT", "OPENGL" };
                static const char *const displaynames[DISPLAY_COUNT] = { "WINDOWED", "BORDERLESS", "FULLSCREEN" };
                static const char *const ssnames[SS_COUNT] = { "OFF", "2K", "4K", "8K" };
                static const char *const filternames[TEXFILTER_COUNT] = { "NEAREST", "BILINEAR", "TRILINEAR" };
                int32 shader = 0, internal = 0, busy = video_pending(), enter, dir = 0;
                int32 renderer = getrendermode(), display = getvideodisplaymode();
                int32 ss = 0, filter = 0, i, vx;
                int32_t gw, gh;
                int rendavail[4] = { 1, 0, 0, 0 }, displayavail[DISPLAY_COUNT] = { 1, 1, 1 };
                int ssavail[SS_COUNT], filteravail[TEXFILTER_COUNT], ssgrey, filtergrey;
                vmode_t res[VIDEOPAGE_MAXRES];
                int nres;
                char buf[32];
                short was;
                const char *note = NULL;

#ifdef POLYMOST
                rendavail[1] = rendavail[2] = 1;
#ifdef USE_OPENGL
                rendavail[3] = !video_glunavailable;
#endif
#endif
#ifdef USE_OPENGL
                shader = (renderer == 3) && (gldrawpath == GLPATH_SHADER);
                internal = (glinternalwidth != 0) && (glinternalheight != 0);
                ss = glsupersample;
                filter = gltexturefilter;
#endif
                for(i=0;i<SS_COUNT;i++) ssavail[i] = videosupersampleavailable(i);
                for(i=0;i<TEXFILTER_COUNT;i++) filteravail[i] = videotexfilteravailable(i);
                // greyed with only off (nearest) to offer, showing the level kept
                ssgrey = !(ssavail[SS_2K] || ssavail[SS_4K] || ssavail[SS_8K]);
                filtergrey = !(filteravail[TEXFILTER_BILINEAR] || filteravail[TEXFILTER_TRILINEAR]);
                video_getmode(&gw,&gh);
                nres = videopage_reslist(display,gw,gh,res);
                c = 46;  // labels (the cursor all on screen at 4:3); values from vx
                vx = c+144;

                rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
                menutext(320>>1,15,0,0,"VIDEO");
                onbar = ( probey == 4 || probey == 5 || probey == 6 );

                x = probe(c,31,15,9);
                enter = (x >= 0) && !onbar;  // Enter, Space or a click
                if(!onbar && x < -1)
                {
                    if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) ) dir = -1;
                    else if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) ) dir = 1;
                    KB_ClearKeyDown( sc_LeftArrow );
                    KB_ClearKeyDown( sc_kpad_4 );
                    KB_ClearKeyDown( sc_RightArrow );
                    KB_ClearKeyDown( sc_kpad_6 );
                    if(dir) sound(KICK_HIT);
                }
                if(probey != 2) videopage_browse = 0;  // the size picked only stays on its row

                if(x == -1)
                {
                    videopage_browse = 0;
                    cmenu(200);
                }
                else if(probey == 2)  // RESOLUTION: arrows pick, Enter applies
                {
                    if(dir && nres > 0 && display != DISPLAY_BORDERLESS)
                    {
                        if(!videopage_browse) { videopage_bw = gw; videopage_bh = gh; }
                        i = videopage_resstep(res,nres,videopage_bw,videopage_bh,dir);
                        videopage_bw = res[i].w;
                        videopage_bh = res[i].h;
                        videopage_browse = 1;
                    }
                    /* Resolution arrows only browse; commit the pending size on activation
                     * when no video transition is busy. */
                    if(enter && videopage_browse && !busy)
                    {
                        if(videopage_bw != gw || videopage_bh != gh)
                        {
                            video_requestmode(display,videopage_bw,videopage_bh);
                            video_pagetouch(VROW_RESOLUTION);
                        }
                        videopage_browse = 0;
                    }
                }
                else
                {
                    if(enter) dir = 1;
                    if(dir && !busy) switch(probey)
                    {
                        case 0:
                            i = videopage_next(renderer,dir,4,rendavail);
                            if(i != renderer)
                            {
                                video_requestrenderer(i);
                                video_pagetouch(VROW_RENDERER);
                            }
                            break;
                        case 1:
                            // from the last choice, which a fallback (from
                            // fullscreen, or BUILD_WINDOWED) doesn't change
                            i = videopage_next(video_pagegetdisplay(),dir,DISPLAY_COUNT,displayavail);
                            if(i != video_pagegetdisplay())
                            {
                                int32 w = gw, h = gh;

                                videopage_modesize(i,&w,&h);
                                video_requestmode(i,w,h);
                                video_pagesetdisplay(i);
                                if(w != gw || h != gh)
                                    video_pagetouch(VROW_RESOLUTION);
                            }
                            break;
                        case 3:
                            setvideovsync(!getvideovsync());
                            video_pagetouch(VROW_VSYNC);
                            break;
                        case 7:
                            i = videopage_next(ss,dir,SS_COUNT,ssavail);
                            if(i != ss && (!ssgrey || i == SS_OFF))  // greyed, a level kept can still go off
                            {
                                video_requestsupersample(i);
                                video_pagetouch(VROW_SUPERSAMPLING);
                            }
                            break;
                        case 8:
                            i = videopage_next(filter,dir,TEXFILTER_COUNT,filteravail);
                            if(i != filter && !filtergrey)
                            {
                                video_requesttexfilter(i);
                                video_pagetouch(VROW_TEXFILTER);
                            }
                            break;
                    }
                }

                i = videopage_next(renderer,1,4,rendavail) == renderer;
                menutext(c,31,0,i,"RENDERER");
                menutext(vx,31,0,i,(char *)renderernames[renderer&3]);

                menutext(c,31+15,0,0,"DISPLAY");
                menutext(vx,31+15,0,0,(char *)displaynames[display]);

                menutext(c,31+15*2,0,display == DISPLAY_BORDERLESS || nres == 0,"RESOLUTION");
                if(display == DISPLAY_BORDERLESS)
                    strcpy(buf,"DESKTOP");
                else if(videopage_browse)
                    sprintf(buf,"%dX%d",(int)videopage_bw,(int)videopage_bh);
                else sprintf(buf,"%dX%d",(int)gw,(int)gh);
                menutext(vx,31+15*2,0,display == DISPLAY_BORDERLESS,buf);

                menutext(c,31+15*3,0,0,"VSYNC");
                menutext(vx,31+15*3,0,0,getvideovsync() ? "ON" : "OFF");

                menutext(c,31+15*4,0,0,"BRIGHTNESS");
                was = shorttmp = (short) ud.brightness;
                bar(vx+18,31+15*4,(short *)&shorttmp,8,x==4,0,0);
                if(shorttmp != was)
                {
                    ud.brightness = (int32) shorttmp;
                    setbrightness(ud.brightness>>2,(unsigned char *)gameviewpalette());
                    video_pagetouch(VROW_BRIGHTNESS);
                }

                // Slider positions 0..60, 1.0 at 30: contrast 0.5..1.5 in
                // steps of 0.1, saturation 0..2 in steps of 0.1. The setting
                // changes only when its slider moves.
                menutext(c,31+15*5,0,!shader,"CONTRAST");
                was = shorttmp = (short) ((GLContrast-GL_CONTRAST_MIN)*60.0+0.5);
                bar(vx+18,31+15*5,(short *)&shorttmp,6,shader && x==5,0,!shader);
                if(shorttmp != was)
                {
                    GLContrast = GL_CONTRAST_MIN+min(shorttmp,60)/60.0;
                    video_pagetouch(VROW_CONTRAST);
                }

                menutext(c,31+15*6,0,!shader,"SATURATION");
                was = shorttmp = (short) (GLSaturation*30.0+0.5);
                bar(vx+18,31+15*6,(short *)&shorttmp,3,shader && x==6,0,!shader);
                if(shorttmp != was)
                {
                    GLSaturation = min(shorttmp,60)/30.0;
                    video_pagetouch(VROW_SATURATION);
                }

#ifdef USE_OPENGL
                glcontrast = GLContrast;
                glsaturation = GLSaturation;
#endif

                menutext(c,31+15*7,0,ssgrey,"SUPERSAMPLE");
                menutext(vx,31+15*7,0,ssgrey,(char *)ssnames[ss&3]);

                menutext(c,31+15*8,0,filtergrey,"FILTERING");
                menutext(vx,31+15*8,0,filtergrey,(char *)filternames[filter%TEXFILTER_COUNT]);

                // A note on the row under the cursor; OpenGL's failure on any.
                switch(probey)
                {
                    case 2:
                        if(display == DISPLAY_BORDERLESS) note = "BORDERLESS FILLS THE SCREEN";
                        else if(videopage_browse) note = "PRESS ENTER TO USE THIS SIZE";
                        break;
                    case 5: case 6:
                        if(!shader) note = "NEEDS THE OPENGL 2.1 SHADER PATH";
                        break;
                    case 7:
                        if(!ssgrey) break;
                        if(!shader) note = "NEEDS THE OPENGL 2.1 SHADER PATH";
                        else if(internal) note = "OFF WHILE AN INTERNAL SIZE IS SET";
                        else note = "NOT LARGER THAN THIS WINDOW";
                        break;
                    case 8:
                        if(filtergrey)
                            note = (renderer == 3) ? "UNAVAILABLE ON THIS DRIVER" : "NEEDS THE OPENGL RENDERER";
                        break;
                }
                if(video_glunavailable)
                    gametext(160,159,"OPENGL COULD NOT START",0,2+8+16);
                if(note)
                    gametext(160,168,(char *)note,0,2+8+16);
            }
            break;

        case 700:
            c = (320>>1)-120;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"SOUNDS");
            onbar = ( probey == 2 || probey == 3 );

            x = probe(c,50,16,7);
            switch(x)
            {
                case -1:
                    if(ps[myconnectindex].gm&MODE_GAME)
                    {
                        ps[myconnectindex].gm &= ~MODE_MENU;
                        if(ud.multimode < 2  && ud.recstat != 2)
                        {
                            ready2send = 1;
                            totalclock = ototalclock;
                        }
                    }

                    else cmenu(200);
                    break;
                case 0:
                    if (FXDevice != NumSoundCards)
                    {
                        SoundToggle = 1-SoundToggle;
                        if( SoundToggle == 0 )
                        {
                            FX_StopAllSounds();
                            clearsoundlocks();
                        }
                        onbar = 0;
                    }
                    break;
                case 1:

                    if(eightytwofifty == 0 || numplayers < 2)
                        if(MusicDevice != NumSoundCards)
                    {
                        MusicToggle = 1-MusicToggle;
                        if( MusicToggle == 0 ) MUSIC_Pause();
                        else
                        {
                            if(ud.recstat != 2 && ps[myconnectindex].gm&MODE_GAME)
                                playmusic(&music_fn[0][(int)music_select][0]);
                            else playmusic(&env_music_fn[0][0]);

                            MUSIC_Continue();
                        }
                    }
                    onbar = 0;

                    break;
                case 4:
                    if(SoundToggle && (FXDevice != NumSoundCards)) VoiceToggle = 1-VoiceToggle;
                    onbar = 0;
                    break;
                case 5:
                    if(SoundToggle && (FXDevice != NumSoundCards)) AmbienceToggle = 1-AmbienceToggle;
                    onbar = 0;
                    break;
                case 6:
                    if(SoundToggle && (FXDevice != NumSoundCards))
                    {
                        ReverseStereo = 1-ReverseStereo;
                        FX_SetReverseStereo(ReverseStereo);
                    }
                    onbar = 0;
                    break;
                default:
                    onbar = 1;
                    break;
            }

            if(SoundToggle && FXDevice != NumSoundCards) menutext(c+160+40,50,0,(FXDevice == NumSoundCards),"ON");
            else menutext(c+160+40,50,0,(FXDevice == NumSoundCards),"OFF");

            if(MusicToggle && (MusicDevice != NumSoundCards) && (!eightytwofifty||numplayers<2))
                menutext(c+160+40,50+16,0,(MusicDevice == NumSoundCards),"ON");
            else menutext(c+160+40,50+16,0,(MusicDevice == NumSoundCards),"OFF");

            menutext(c,50,SHX(-2),(FXDevice == NumSoundCards),"SOUND");
            menutext(c,50+16+16,SHX(-4),(FXDevice==NumSoundCards)||SoundToggle==0,"SOUND VOLUME");
            {
                l = FXVolume;
                FXVolume >>= 2;
                shorttmp = (short) FXVolume;
                bar(c+167+40,50+16+16,(short *)&shorttmp,4,(FXDevice!=NumSoundCards)&&x==2,SHX(-4),SoundToggle==0||(FXDevice==NumSoundCards));
                FXVolume = (int32) shorttmp;
                if(l != FXVolume)
                    FXVolume <<= 2;
                if(l != FXVolume)
                    FX_SetVolume( (short) FXVolume );
            }
            menutext(c,50+16,SHX(-3),(MusicDevice==NumSoundCards),"MUSIC");
            menutext(c,50+16+16+16,SHX(-5),(MusicDevice==NumSoundCards)||(numplayers > 1 && eightytwofifty)||MusicToggle==0,"MUSIC VOLUME");
            {
                l = MusicVolume;
                MusicVolume >>= 2;
                shorttmp = (short) MusicVolume;
                bar(c+167+40,50+16+16+16,
                    &shorttmp,4,
                    (eightytwofifty==0||numplayers < 2) && (MusicDevice!=NumSoundCards) && x==3,SHX(-5),
                    (numplayers > 1 && eightytwofifty)||MusicToggle==0||(MusicDevice==NumSoundCards));
                MusicVolume = (int32) shorttmp;
                MusicVolume <<= 2;
                if(l != MusicVolume)
                {
                    STUBBED("Check this");
                    // !!! FIXME: Used to be Music_ not MUSIC_.  --ryan.
                    MUSIC_SetVolume( (short) MusicVolume );
                }
            }
            menutext(c,50+16+16+16+16,SHX(-6),(FXDevice==NumSoundCards)||SoundToggle==0,"DUKE TALK");
            menutext(c,50+16+16+16+16+16,SHX(-7),(FXDevice==NumSoundCards)||SoundToggle==0,"AMBIENCE");

            menutext(c,50+16+16+16+16+16+16,SHX(-8),(FXDevice==NumSoundCards)||SoundToggle==0,"FLIP STEREO");

            if(VoiceToggle) menutext(c+160+40,50+16+16+16+16,0,(FXDevice==NumSoundCards)||SoundToggle==0,"ON");
            else menutext(c+160+40,50+16+16+16+16,0,(FXDevice==NumSoundCards)||SoundToggle==0,"OFF");

            if(AmbienceToggle) menutext(c+160+40,50+16+16+16+16+16,0,(FXDevice==NumSoundCards)||SoundToggle==0,"ON");
            else menutext(c+160+40,50+16+16+16+16+16,0,(FXDevice==NumSoundCards)||SoundToggle==0,"OFF");

            if(ReverseStereo) menutext(c+160+40,50+16+16+16+16+16+16,0,(FXDevice==NumSoundCards)||SoundToggle==0,"ON");
            else menutext(c+160+40,50+16+16+16+16+16+16,0,(FXDevice==NumSoundCards)||SoundToggle==0,"OFF");


            break;

        /* Capture the game view once before entering the save-slot browser. */
        case 350:
            cmenu(351);
            screencapt = 1;
            displayrooms(myconnectindex,65536);
            savetemp("duke3d.tmp",waloff[MAXTILES-1],160*100);
            screencapt = 0;
            break;

        case 360:
        case 361:
        case 362:
        case 363:
        case 364:
        case 365:
        case 366:
        case 367:
        case 368:
        case 369:
        case 351:
        case 300:

            c = 320>>1;
            if (widescreen) clearallviews(0L);
            rotatesprite(c<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);

            if(current_menu == 300) menutext(c,24,0,0,"LOAD GAME");
            else menutext(c,24,0,0,"SAVE GAME");

            /* These page numbers encode the slot whose save name is being edited. */
            if(current_menu >= 360 && current_menu <= 369 )
            {
#if DUKE3D_GAMEVER >= GAMEVER_10
                sprintf(tempbuf,"PLAYERS: %-2d                      ",ud.multimode);
                gametext(160,158,tempbuf,0,2+8+16);
#endif
#if DUKE3D_GAMEVER < GAMEVER_10
                sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill+1);
#else
                sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
#endif
                gametext(160,170,tempbuf,0,2+8+16);

                x = strget((320>>1),184,&ud.savegame[current_menu-360][0],19, 999 );

                if(x == -1)
                {
            //        readsavenames();
                    ps[myconnectindex].gm = MODE_GAME;
                    if(ud.multimode < 2  && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                    goto DISPLAYNAMES;
                }

                if( x == 1 )
                {
                    if( ud.savegame[current_menu-360][0] == 0 )
                    {
                        KB_FlushKeyboardQueue();
                        cmenu(351);
                    }
                    else
                    {
                        if(ud.multimode > 1)
                            saveplayer(-1-(current_menu-360));
                        else saveplayer(current_menu-360);
                        lastsavedpos = current_menu-360;
                        ps[myconnectindex].gm = MODE_GAME;

                        if(ud.multimode < 2  && ud.recstat != 2)
                        {
                            ready2send = 1;
                            totalclock = ototalclock;
                        }
                        KB_ClearKeyDown(sc_Escape);
                        sound(EXITMENUSOUND);
                    }
                }

                rotatesprite(101<<16,97<<16,65536,512,MAXTILES-1,-32,0,2+4+8+64,0,0,xdim-1,ydim-1);
                dispnames();
                rotatesprite((c+67+(int32_t)strlen(&ud.savegame[current_menu-360][0])*4)<<16,(50+12*probey)<<16,32768L-10240,0,SPINNINGNUKEICON+(((totalclock)>>3)%7),0,0,10,0,0,xdim-1,ydim-1);
                break;
            }

           last_threehundred = probey;

            x = probe(c+68,54,12,10);

          if(current_menu == 300)
          {
              if( ud.savegame[probey][0] )
              {
                  if( lastprobey != probey )
                  {
                     loadpheader(probey,&volnum,&levnum,&plrskl,&numplr);
                     lastprobey = probey;
                  }

                  rotatesprite(101<<16,97<<16,65536L,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
#if DUKE3D_GAMEVER >= GAMEVER_10
                  sprintf(tempbuf,"PLAYERS: %-2d                      ",numplr);
                  gametext(160,158,tempbuf,0,2+8+16);
#endif
#if DUKE3D_GAMEVER < GAMEVER_10
                  sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+volnum,1+levnum,plrskl+1);
#else
                  sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+volnum,1+levnum,plrskl);
#endif
                  gametext(160,170,tempbuf,0,2+8+16);
              }
              else menutext(69,70,0,0,"EMPTY");
          }
          else
          {
              if( ud.savegame[probey][0] )
              {
                  if(lastprobey != probey)
                      loadpheader(probey,&volnum,&levnum,&plrskl,&numplr);
                  lastprobey = probey;
                  rotatesprite(101<<16,97<<16,65536L,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
              }
              else menutext(69,70,0,0,"EMPTY");
#if DUKE3D_GAMEVER >= GAMEVER_10
              sprintf(tempbuf,"PLAYERS: %-2d                      ",ud.multimode);
              gametext(160,158,tempbuf,0,2+8+16);
#endif
#if DUKE3D_GAMEVER < GAMEVER_10
              sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill+1);
#else
              sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
#endif
              gametext(160,170,tempbuf,0,2+8+16);
          }

            switch( x )
            {
                case -1:
                    if(current_menu == 300)
                    {
                        if( (ps[myconnectindex].gm&MODE_GAME) != MODE_GAME)
                        {
                            cmenu(0);
                            break;
                        }
                        else
                            ps[myconnectindex].gm &= ~MODE_MENU;
                    }
                    else
                        ps[myconnectindex].gm = MODE_GAME;

                    if(ud.multimode < 2 && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }

                    break;
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                    if( current_menu == 300)
                    {
                        if( ud.savegame[x][0] )
                            current_menu = (1000+x);
                    }
                    else
                    {
                        if( ud.savegame[x][0] != 0)
                            current_menu = 2000+x;
                        else
                        {
                            KB_FlushKeyboardQueue();
                            current_menu = (360+x);
                            ud.savegame[x][0] = 0;
                            inputloc = 0;
                        }
                    }
                    break;
            }

            DISPLAYNAMES:
            dispnames();
            break;

#ifndef VOLUMEALL
        case 400:
        case 401:
        case 402:
        case 403:

            c = 320>>1;

#if DUKE3D_GAMEVER < GAMEVER_10
            if(KB_KeyPressed(sc_Escape))
            { cmenu((ps[myconnectindex].gm&MODE_GAME)?50:0); return; }
            if(KB_KeyWaiting())
            {
                KB_Getch();
                current_menu++;
                if(current_menu > 403)
                { cmenu((ps[myconnectindex].gm&MODE_GAME)?50:0); return; }
            }
#else
            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < 400) current_menu = 403;
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
                sound(KICK_HIT);
                current_menu++;
                if(current_menu > 403) current_menu = 400;
            }

            if( KB_KeyPressed(sc_Escape) )
            {
                if(ps[myconnectindex].gm&MODE_GAME)
                    cmenu(50);
                else cmenu(0);
                return;
            }

#endif
            flushperms();
            if (widescreen) clearallviews(0L);
            rotatesprite(0,0,65536L,0,ORDERING+current_menu-400,0,0,10+16+64,0,0,xdim-1,ydim-1);

#else
        case 400:
        case 401:

            c = 320>>1;

            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < 400) current_menu = 401;
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
                sound(KICK_HIT);
                current_menu++;
                if(current_menu > 401) current_menu = 400;
            }

            if( KB_KeyPressed(sc_Escape) )
            {
                if(ps[myconnectindex].gm&MODE_GAME)
                    cmenu(50);
                else cmenu(0);
                return;
            }

            flushperms();
            if (widescreen) clearallviews(0L);
            switch(current_menu)
            {
                case 400:
                    rotatesprite(0,0,65536L,0,TEXTSTORY,0,0,10+16+64, 0,0,xdim-1,ydim-1);
                    break;
                case 401:
                    rotatesprite(0,0,65536L,0,F1HELP,0,0,10+16+64, 0,0,xdim-1,ydim-1);
                    break;
            }

#endif

            break;

        case 500:
            c = 320>>1;

            gametext(c,90,"Are you sure you want to quit?",0,2+8+16);
            gametext(c,99,"(Y/N)",0,2+8+16);

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();

                if( gamequit == 0 && ( numplayers > 1 ) )
                {
                    if(ps[myconnectindex].gm&MODE_GAME)
                    {
                        gamequit = 1;
                        quittimer = totalclock+120;
                    }
                    else
                    {
                        sendlogoff();
                        gameexit(" ");
                    }
                }
                else if( numplayers < 2 )
                    gameexit(" ");

                if( ( totalclock > quittimer ) && ( gamequit == 1) )
                    gameexit("Timed out.");
            }

            x = probe(186,124,0,0);
            if(x == -1 || KB_KeyPressed(sc_N) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                quittimer = 0;
                if( ps[myconnectindex].gm&MODE_DEMO )
                    ps[myconnectindex].gm = MODE_DEMO;
                else
                {
                    ps[myconnectindex].gm &= ~MODE_MENU;
                    if(ud.multimode < 2  && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                }
            }

            break;
        case 501:
            c = 320>>1;
            gametext(c,90,"Quit to Title?",0,2+8+16);
            gametext(c,99,"(Y/N)",0,2+8+16);

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                ps[myconnectindex].gm = MODE_DEMO;
#if DUKE3D_GAMEVER >= GAMEVER_10
                if(ud.recstat == 1)
                    closedemowrite();
#endif
                cmenu(0);
            }

            x = probe(186,124,0,0);

            if(x == -1 || KB_KeyPressed(sc_N) || RMB)
            {
                ps[myconnectindex].gm &= ~MODE_MENU;
                if(ud.multimode < 2  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            break;

        case 601:
            displayfragbar();
            rotatesprite(160<<16,29<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,34,0,0,&ud.user_name[myconnectindex][0]);

            sprintf(tempbuf,"Waiting for master");
            gametext(160,50,tempbuf,0,2+8+16);
            gametext(160,59,"to select level",0,2+8+16);

            if( KB_KeyPressed(sc_Escape) )
            {
                KB_ClearKeyDown(sc_Escape);
                sound(EXITMENUSOUND);
                cmenu(0);
            }
            break;

        case 602:
            if(menunamecnt == 0)
            {
        //        getfilenames("SUBD");
                getfilenames("*.MAP");
                sortfilenames();
                if (menunamecnt == 0)
                    cmenu(600);
            }
        case 603:
            c = (320>>1) - 120;
            displayfragbar();
            rotatesprite(320>>1<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"USER MAPS");
            for(x=0;x<menunamecnt;x++)
            {
                if(x == fileselect)
                    minitext(15 + (x/15)*54,32 + (x%15)*8,menuname[x],0,26);
                else minitext(15 + (x/15)*54,32 + (x%15)*8,menuname[x],16,26);
            }

            fileselect = probey;
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );
                probey -= 15;
                if(probey < 0) probey += 15;
                else sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ) )
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );
                probey += 15;
                if(probey >= menunamecnt)
                    probey -= 15;
                else sound(KICK_HIT);
            }

            onbar = 0;
            x = probe(0,0,0,menunamecnt);

            if(x == -1) cmenu(600);
            else if(x >= 0)
            {
                tempbuf[0] = 8;
                tempbuf[1] = ud.m_level_number = 6;
                tempbuf[2] = ud.m_volume_number = 0;
                tempbuf[3] = ud.m_player_skill+1;

                if(ud.player_skill == 3)
                    ud.m_respawn_monsters = 1;
                else ud.m_respawn_monsters = 0;

                if(ud.m_coop == 0) ud.m_respawn_items = 1;
                else ud.m_respawn_items = 0;

                ud.m_respawn_inventory = 1;

                tempbuf[4] = ud.m_monsters_off;
                tempbuf[5] = ud.m_respawn_monsters;
                tempbuf[6] = ud.m_respawn_items;
                tempbuf[7] = ud.m_respawn_inventory;
                tempbuf[8] = ud.m_coop;
                tempbuf[9] = ud.m_marker;

                x = strlen(menuname[probey]);

                copybufbyte(menuname[probey],tempbuf+10,x);
                copybufbyte(menuname[probey],boardfilename,x+1);

                for(c=connecthead;c>=0;c=connectpoint2[c])
                    if(c != myconnectindex)
                        sendpacket(c,tempbuf,x+10);

                newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                enterlevel(MODE_GAME);
            }
            break;

        case 600:
            c = (320>>1) - 120;
            if((ps[myconnectindex].gm&MODE_GAME) != MODE_GAME)
                displayfragbar();
            rotatesprite(160<<16,26<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,31,0,0,&ud.user_name[myconnectindex][0]);

            x = probe(c,57-8,16,8);

            switch(x)
            {
                case -1:
                    ud.m_recstat = 0;
                    if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);
                    else cmenu(0);
                    break;
                case 0:
                    ud.m_coop++;
                    if(ud.m_coop == 3) ud.m_coop = 0;
                    break;
                case 1:
#ifndef VOLUMEONE
                    ud.m_volume_number++;
#ifdef PLUTOPAK
                    if(ud.m_volume_number > 3) ud.m_volume_number = 0;
#else
                    if(ud.m_volume_number > 2) ud.m_volume_number = 0;
#endif
                    if(ud.m_volume_number == 0 && ud.m_level_number > 6)
                        ud.m_level_number = 0;
                    if(ud.m_level_number > 10) ud.m_level_number = 0;
#endif
                    break;
                case 2:
#ifndef ONELEVELDEMO
                    ud.m_level_number++;
#ifndef VOLUMEONE
                    if(ud.m_volume_number == 0 && ud.m_level_number > 6)
                        ud.m_level_number = 0;
#else
                    if(ud.m_volume_number == 0 && ud.m_level_number > 5)
                        ud.m_level_number = 0;
#endif
                    if(ud.m_level_number > 10) ud.m_level_number = 0;
#endif
                    break;
                case 3:
                    if(ud.m_monsters_off == 1 && ud.m_player_skill > 0)
                        ud.m_monsters_off = 0;

                    if(ud.m_monsters_off == 0)
                    {
                        ud.m_player_skill++;
                        if(ud.m_player_skill > 3)
                        {
                            ud.m_player_skill = 0;
                            ud.m_monsters_off = 1;
                        }
                    }
                    else ud.m_monsters_off = 0;

                    break;

                case 4:
                    if(ud.m_coop == 0)
                        ud.m_marker = !ud.m_marker;
                    break;

                case 5:
                    if(ud.m_coop == 1)
                        ud.m_ffire = !ud.m_ffire;
                    break;

                case 6:
#ifdef VOLUMEALL
                    if(boardfilename[0] == 0) break;

                    tempbuf[0] = 5;
                    tempbuf[1] = ud.m_level_number = 7;
                    tempbuf[2] = ud.m_volume_number = 0;
                    tempbuf[3] = ud.m_player_skill+1;

                    ud.level_number = ud.m_level_number;
                    ud.volume_number = ud.m_volume_number;

                    if( ud.m_player_skill == 3 ) ud.m_respawn_monsters = 1;
                    else ud.m_respawn_monsters = 0;

                    if(ud.m_coop == 0) ud.m_respawn_items = 1;
                    else ud.m_respawn_items = 0;

                    ud.m_respawn_inventory = 1;

                    tempbuf[4] = ud.m_monsters_off;
                    tempbuf[5] = ud.m_respawn_monsters;
                    tempbuf[6] = ud.m_respawn_items;
                    tempbuf[7] = ud.m_respawn_inventory;
                    tempbuf[8] = ud.m_coop;
                    tempbuf[9] = ud.m_marker;
                    tempbuf[10] = ud.m_ffire;

                    for(c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        resetweapons(c);
                        resetinventory(c);

                        if(c != myconnectindex)
                            sendpacket(c,tempbuf,11);
                    }

                    newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                    enterlevel(MODE_GAME);

                    return;
#endif
                case 7:

                    tempbuf[0] = 5;
                    tempbuf[1] = ud.m_level_number;
                    tempbuf[2] = ud.m_volume_number;
                    tempbuf[3] = ud.m_player_skill+1;

                    if( ud.m_player_skill == 3 ) ud.m_respawn_monsters = 1;
                    else ud.m_respawn_monsters = 0;

                    if(ud.m_coop == 0) ud.m_respawn_items = 1;
                    else ud.m_respawn_items = 0;

                    ud.m_respawn_inventory = 1;

                    tempbuf[4] = ud.m_monsters_off;
                    tempbuf[5] = ud.m_respawn_monsters;
                    tempbuf[6] = ud.m_respawn_items;
                    tempbuf[7] = ud.m_respawn_inventory;
                    tempbuf[8] = ud.m_coop;
                    tempbuf[9] = ud.m_marker;
                    tempbuf[10] = ud.m_ffire;

                    for(c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        resetweapons(c);
                        resetinventory(c);

                        if(c != myconnectindex)
                            sendpacket(c,tempbuf,11);
                    }

                    newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                    enterlevel(MODE_GAME);

                    return;

            }

            c += 40;

            if(ud.m_coop==1) gametext(c+70,57-7-9,"COOPERATIVE PLAY",0,2+8+16);
            else if(ud.m_coop==2) gametext(c+70,57-7-9,"DUKEMATCH (NO SPAWN)",0,2+8+16);
            else gametext(c+70,57-7-9,"DUKEMATCH (SPAWN)",0,2+8+16);

 #ifdef VOLUMEONE
            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],0,2+8+16);
 #else
            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],0,2+8+16);
 #endif

            gametext(c+70,57+16+16-7-9,&level_names[11*ud.m_volume_number+ud.m_level_number][0],0,2+8+16);

            if(ud.m_monsters_off == 0 || ud.m_player_skill > 0)
                gametext(c+70,57+16+16+16-7-9,skill_names[ud.m_player_skill],0,2+8+16);
            else gametext(c+70,57+16+16+16-7-9,"NONE",0,2+8+16);

            if(ud.m_coop == 0)
            {
                if(ud.m_marker)
                    gametext(c+70,57+16+16+16+16-7-9,"ON",0,2+8+16);
                else gametext(c+70,57+16+16+16+16-7-9,"OFF",0,2+8+16);
            }

            if(ud.m_coop == 1)
            {
                if(ud.m_ffire)
                    gametext(c+70,57+16+16+16+16+16-7-9,"ON",0,2+8+16);
                else gametext(c+70,57+16+16+16+16+16-7-9,"OFF",0,2+8+16);
            }

            c -= 44;

            menutext(c,57-9,SHX(-2),PHX(-2),"GAME TYPE");

#ifdef VOLUMEONE
            sprintf(tempbuf,"EPISODE %d",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),1,tempbuf);
#else
            sprintf(tempbuf,"EPISODE %d",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),PHX(-3),tempbuf);
#endif

#ifndef ONELEVELDEMO
            sprintf(tempbuf,"LEVEL %d",ud.m_level_number+1);
            menutext(c,57+16+16-9,SHX(-4),PHX(-4),tempbuf);
#else
            sprintf(tempbuf,"LEVEL %d",ud.m_level_number+1);
            menutext(c,57+16+16-9,SHX(-4),1,tempbuf);
#endif
            menutext(c,57+16+16+16-9,SHX(-5),PHX(-5),"MONSTERS");

            if(ud.m_coop == 0)
                menutext(c,57+16+16+16+16-9,SHX(-6),PHX(-6),"MARKERS");
            else
                menutext(c,57+16+16+16+16-9,SHX(-6),1,"MARKERS");

            if(ud.m_coop == 1)
                menutext(c,57+16+16+16+16+16-9,SHX(-6),PHX(-6),"FR. FIRE");
            else menutext(c,57+16+16+16+16+16-9,SHX(-6),1,"FR. FIRE");

#ifdef VOLUMEALL
            menutext(c,57+16+16+16+16+16+16-9,SHX(-7),boardfilename[0] == 0,"USER MAP");
            if( boardfilename[0] != 0 )
                gametext(c+70+44,57+16+16+16+16+16,boardfilename,0,2+8+16);
#else
            menutext(c,57+16+16+16+16+16+16-9,SHX(-7),1,"USER MAP");
#endif

            menutext(c,57+16+16+16+16+16+16+16-9,SHX(-8),PHX(-8),"START GAME");

            break;
    }

    if( (ps[myconnectindex].gm&MODE_MENU) != MODE_MENU)
    {
        vscrn();
        cameraclock = totalclock;
        cameradist = 65536L;
    }
}

/* The game palette and brightness renderer 3 last applied (NULL: unknown). */
static char *glgamepalette;
static int32_t glgamebrightness;

/* The player palette on screen: in a game or demo the viewed player's
   (screenpeek), elsewhere myconnectindex's (see glsetgamepalette). */
/*
================
gameviewpalette

Choose the viewed player's palette during play or demos, except at level end.
Use the local player's palette for the surrounding screens.
================
*/
char *gameviewpalette(void)
{
    return ps[((ps[myconnectindex].gm&(MODE_GAME|MODE_DEMO)) &&
               !(ps[myconnectindex].gm&MODE_EOL)) ? screenpeek : myconnectindex].palette;
}

/* After a mode set, which resets the palette: glsetgamepalette applies it again. */
void glresetgamepalette(void)
{
    glgamepalette = NULL;
}

/*
 * Renderer 3 bakes the palette into textures as it draws, so a new player
 *  palette must reach the engine before the next draw, not at the next
 *  palto. Applies it only when it changed, keeping any fade. In a game or
 *  demo it's the viewed player's (screenpeek), as restorepalette sets;
 *  elsewhere (title, bonus and ending screens, and at a level's end) the
 *  game sets myconnectindex's.
 */
void glsetgamepalette(void)
{
#ifdef POLYMOST
    palette_t fadergb;
    unsigned char fadedelta;
    char *pal = gameviewpalette();
#ifdef USE_OPENGL
    /* Lights glow in the game's palettes, not in the title, ending or 3DR ones. */
    int32_t fullbright = (pal == (char *)palette || pal == waterpal || pal == slimepal) ? 240 : 256;
#endif

    if (getrendermode() != 3) return;
    if (pal == glgamepalette && ud.brightness == glgamebrightness
#ifdef USE_OPENGL
        && fullbright == polymosttexfullbright
#endif
        )
        return;

#ifdef USE_OPENGL
    /* setbrightness retextures only if the palette changed; the GL 2.1 tier
       has no RGB textures, and its shade tables keep lights lit. */
    if ((fullbright != polymosttexfullbright) && (gldrawpath != GLPATH_SHADER)) gltexinvalidateall();
    polymosttexfullbright = fullbright;
#endif
    glgamepalette = pal;
    glgamebrightness = ud.brightness;
    fadergb = palfadergb;  /* setbrightness clears the fade. */
    fadedelta = palfadedelta;
    setbrightness(ud.brightness>>2,(unsigned char *)glgamepalette);
    palfadergb = fadergb;
    palfadedelta = fadedelta;
#endif
}

/*
================
palto

Blend toward a six-bit RGB colour using the low seven bits of e as strength.
The high bit bypasses rate limiting; renderer 3 applies a fade overlay.
================
*/
void palto(char r,char g,char b,int32_t e)
{
    int i;
    char temparray[768];

    /* Palette channels are six-bit values. A blend strength of 64 reaches
     * the target colour through the integer divide by 64. */
    for(i=0;i<768;i+=3)
    {
        temparray[i  ] =
            ps[myconnectindex].palette[i+0]+((((int32_t)r-(int32_t)ps[myconnectindex].palette[i+0])*(int32_t)(e&127))>>6);
        temparray[i+1] =
            ps[myconnectindex].palette[i+1]+((((int32_t)g-(int32_t)ps[myconnectindex].palette[i+1])*(int32_t)(e&127))>>6);
        temparray[i+2] =
            ps[myconnectindex].palette[i+2]+((((int32_t)b-(int32_t)ps[myconnectindex].palette[i+2])*(int32_t)(e&127))>>6);
    }

// CTW - MODIFICATION
/*  if( (e&128) == 0 )
        if ((vidoption != 1) || (vgacompatible == 1)) limitrate();*/
    if( (e&128) == 0 )
        if ((ScreenMode != 1) || (vgacompatible == 1)) limitrate();
// CTW END - MODIFICATION

    if (getrendermode() == 3)
    {
        /* a fade quad over the frame, not a palette (and texture) change. */
        glsetgamepalette();
        setpalettefade(min(r,63),min(g,63),min(b,63),e&127);
        return;
    }

    setbrightness(ud.brightness>>2,temparray);
}


/*
================
drawoverheadmap

Project discovered map geometry, selected sprites and player markers
into the rotated and zoomed overhead view.
================
*/
void drawoverheadmap(int32_t cposx, int32_t cposy, int32_t czoom, short cang)
{
        int32_t i=0l, j=0l, k=0l, l=0l, x1=0l, y1=0l, x2=0l, y2=0l, x3=0l;
        int32_t y3=0l, x4=0l, y4=0l, ox=0l, oy=0l, xoff=0l, yoff=0l;
        int32_t dax=0l, day=0l, cosang=0l, sinang=0l, xspan=0l, yspan=0l, sprx=0l, spry=0l;
        int32_t xrepeat=0l, yrepeat=0l, z1=0l, z2=0l, startwall=0l, endwall=0l, tilenum=0l, daang=0l;
        int32_t xvect=0l, yvect=0l, xvect2=0l, yvect2=0l;
        short p=0l;
        char col;
        walltype *wal, *wal2;
        spritetype *spr;

        /* Fold rotation and zoom into the basis vectors; the second pair also
         * corrects the screen aspect before coordinates reach drawline256. */
        xvect = sintable[(-cang)&2047] * czoom;
        yvect = sintable[(1536-cang)&2047] * czoom;
        xvect2 = mulscale16(xvect,yxaspect);
        yvect2 = mulscale16(yvect,yxaspect);

                //Draw red lines
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

                startwall = sector[i].wallptr;
                endwall = sector[i].wallptr + sector[i].wallnum;

                z1 = sector[i].ceilingz; z2 = sector[i].floorz;

                for(j=startwall,wal=&wall[startwall];j<endwall;j++,wal++)
                {
                        k = wal->nextwall; if (k < 0) continue;

                        //if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;
                        //if ((k > j) && ((show2dwall[k>>3]&(1<<(k&7))) > 0)) continue;

                        if (sector[wal->nextsector].ceilingz == z1)
                                if (sector[wal->nextsector].floorz == z2)
                                        if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

                        col = 139; //red
                        if ((wal->cstat|wall[wal->nextwall].cstat)&1) col = 234; //magenta

                        if (!(show2dsector[wal->nextsector>>3]&(1<<(wal->nextsector&7))))
                                col = 24;
            else continue;

                        ox = wal->x-cposx; oy = wal->y-cposy;
                        x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        wal2 = &wall[wal->point2];
                        ox = wal2->x-cposx; oy = wal2->y-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        drawline256(x1,y1,x2,y2,col);
                }
        }

                //Draw sprites
        k = ps[screenpeek].i;
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;
                for(j=headspritesect[i];j>=0;j=nextspritesect[j])
                        //if ((show2dsprite[j>>3]&(1<<(j&7))) > 0)
                        {
                spr = &sprite[j];

                if (j == k || (spr->cstat&0x8000) || spr->cstat == 257 || spr->xrepeat == 0) continue;

                                col = 71; //cyan
                                if (spr->cstat&1) col = 234; //magenta

                                sprx = spr->x;
                                spry = spr->y;

                if( (spr->cstat&257) != 0) switch (spr->cstat&48)
                                {
                    case 0: break;

                                                ox = sprx-cposx; oy = spry-cposy;
                                                x1 = dmulscale16(ox,xvect,-oy,yvect);
                                                y1 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = (sintable[(spr->ang+512)&2047]>>7);
                                                oy = (sintable[(spr->ang)&2047]>>7);
                                                x2 = dmulscale16(ox,xvect,-oy,yvect);
                                                y2 = dmulscale16(oy,xvect,ox,yvect);

                                                x3 = mulscale16(x2,yxaspect);
                                                y3 = mulscale16(y2,yxaspect);

                                                drawline256(x1-x2+(xdim<<11),y1-y3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                                                drawline256(x1-y2+(xdim<<11),y1+x3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                                                drawline256(x1+y2+(xdim<<11),y1-x3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                        break;

                                        case 16:
                        if( spr->picnum == LASERLINE )
                        {
                            x1 = sprx; y1 = spry;
                            tilenum = spr->picnum;
                            xoff = (int32_t)((signed char)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                            if ((spr->cstat&4) > 0) xoff = -xoff;
                            k = spr->ang; l = spr->xrepeat;
                            dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
                            l = tilesizx[tilenum]; k = (l>>1)+xoff;
                            x1 -= mulscale16(dax,k); x2 = x1+mulscale16(dax,l);
                            y1 -= mulscale16(day,k); y2 = y1+mulscale16(day,l);

                            ox = x1-cposx; oy = y1-cposy;
                            x1 = dmulscale16(ox,xvect,-oy,yvect);
                            y1 = dmulscale16(oy,xvect2,ox,yvect2);

                            ox = x2-cposx; oy = y2-cposy;
                            x2 = dmulscale16(ox,xvect,-oy,yvect);
                            y2 = dmulscale16(oy,xvect2,ox,yvect2);

                            drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                                                                x2+(xdim<<11),y2+(ydim<<11),col);
                        }

                        break;

                    case 32:

                                                tilenum = spr->picnum;
                                                xoff = (int32_t)((signed char)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                                                yoff = (int32_t)((signed char)((picanm[tilenum]>>16)&255))+((int32_t)spr->yoffset);
                                                if ((spr->cstat&4) > 0) xoff = -xoff;
                                                if ((spr->cstat&8) > 0) yoff = -yoff;

                                                k = spr->ang;
                                                cosang = sintable[(k+512)&2047]; sinang = sintable[k];
                                                xspan = tilesizx[tilenum]; xrepeat = spr->xrepeat;
                                                yspan = tilesizy[tilenum]; yrepeat = spr->yrepeat;

                                                dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
                                                x1 = sprx + dmulscale16(sinang,dax,cosang,day);
                                                y1 = spry + dmulscale16(sinang,day,-cosang,dax);
                                                l = xspan*xrepeat;
                                                x2 = x1 - mulscale16(sinang,l);
                                                y2 = y1 + mulscale16(cosang,l);
                                                l = yspan*yrepeat;
                                                k = -mulscale16(cosang,l); x3 = x2+k; x4 = x1+k;
                                                k = -mulscale16(sinang,l); y3 = y2+k; y4 = y1+k;

                                                ox = x1-cposx; oy = y1-cposy;
                                                x1 = dmulscale16(ox,xvect,-oy,yvect);
                                                y1 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x2-cposx; oy = y2-cposy;
                                                x2 = dmulscale16(ox,xvect,-oy,yvect);
                                                y2 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x3-cposx; oy = y3-cposy;
                                                x3 = dmulscale16(ox,xvect,-oy,yvect);
                                                y3 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x4-cposx; oy = y4-cposy;
                                                x4 = dmulscale16(ox,xvect,-oy,yvect);
                                                y4 = dmulscale16(oy,xvect2,ox,yvect2);

                                                drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                                                                x2+(xdim<<11),y2+(ydim<<11),col);

                                                drawline256(x2+(xdim<<11),y2+(ydim<<11),
                                                                                x3+(xdim<<11),y3+(ydim<<11),col);

                                                drawline256(x3+(xdim<<11),y3+(ydim<<11),
                                                                                x4+(xdim<<11),y4+(ydim<<11),col);

                                                drawline256(x4+(xdim<<11),y4+(ydim<<11),
                                                                                x1+(xdim<<11),y1+(ydim<<11),col);

                                                break;
                                }
                        }
        }

                //Draw white lines
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

                startwall = sector[i].wallptr;
                endwall = sector[i].wallptr + sector[i].wallnum;

                k = -1;
                for(j=startwall,wal=&wall[startwall];j<endwall;j++,wal++)
                {
                        if (wal->nextwall >= 0) continue;

                        //if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;

                        if (tilesizx[wal->picnum] == 0) continue;
                        if (tilesizy[wal->picnum] == 0) continue;

                        if (j == k)
                                { x1 = x2; y1 = y2; }
                        else
                        {
                                ox = wal->x-cposx; oy = wal->y-cposy;
                                x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                                y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);
                        }

                        k = wal->point2; wal2 = &wall[k];
                        ox = wal2->x-cposx; oy = wal2->y-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        drawline256(x1,y1,x2,y2,24);
                }
        }

         for(p=connecthead;p >= 0;p=connectpoint2[p])
         {
          if(ud.scrollmode && p == screenpeek) continue;

          ox = sprite[ps[p].i].x-cposx; oy = sprite[ps[p].i].y-cposy;
                  daang = (sprite[ps[p].i].ang-cang)&2047;
                  if (p == screenpeek) { ox = 0; oy = 0; daang = 0; }
                  x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
                  y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

          if(p == screenpeek || ud.coop == 1 )
          {
                if(sprite[ps[p].i].xvel > 16 && ps[p].on_ground)
                    i = APLAYERTOP+((totalclock>>4)&3);
                else
                    i = APLAYERTOP;

                j = klabs(ps[p].truefz-ps[p].posz)>>8;
                j = mulscale(czoom*(sprite[ps[p].i].yrepeat+j),yxaspect,16);

                if(j < 22000) j = 22000;
                else if(j > (65536<<1)) j = (65536<<1);

                rotatesprite((x1<<4)+(xdim<<15),(y1<<4)+(ydim<<15),j,
                    daang,i,sprite[ps[p].i].shade,sprite[ps[p].i].pal,
                    (sprite[ps[p].i].cstat&2)>>1,windowx1,windowy1,windowx2,windowy2);
          }
         }
}



void endanimsounds(int32_t fr)
{
    switch(ud.volume_number)
    {
        case 0:break;
        case 1:
            switch(fr)
            {
                case 1:
                    sound(WIND_AMBIENCE);
                    break;
                case 26:
                    sound(ENDSEQVOL2SND1);
                    break;
                case 36:
                    sound(ENDSEQVOL2SND2);
                    break;
                case 54:
                    sound(THUD);
                    break;
                case 62:
                    sound(ENDSEQVOL2SND3);
                    break;
                case 75:
                    sound(ENDSEQVOL2SND4);
                    break;
                case 81:
                    sound(ENDSEQVOL2SND5);
                    break;
                case 115:
                    sound(ENDSEQVOL2SND6);
                    break;
                case 124:
                    sound(ENDSEQVOL2SND7);
                    break;
            }
            break;
        case 2:
            switch(fr)
            {
                case 1:
                    sound(WIND_REPEAT);
                    break;
                case 98:
                    sound(DUKE_GRUNT);
                    break;
                case 82+20:
                    sound(THUD);
                    sound(SQUISHED);
                    break;
                case 104+20:
                    sound(ENDSEQVOL3SND3);
                    break;
                case 114+20:
                    sound(ENDSEQVOL3SND2);
                    break;
                case 158:
                    sound(PIPEBOMB_EXPLODE);
                    break;
            }
            break;
    }
}

void logoanimsounds(int32_t fr)
{
    switch(fr)
    {
        case 1:
            sound(FLY_BY);
            break;
        case 19:
            sound(PIPEBOMB_EXPLODE);
            break;
    }
}

void intro4animsounds(int32_t fr)
{
    switch(fr)
    {
        case 1:
            sound(INTRO4_B);
            break;
        case 12:
        case 34:
            sound(SHORT_CIRCUIT);
            break;
        case 18:
            sound(INTRO4_5);
            break;
    }
}

void first4animsounds(int32_t fr)
{
    switch(fr)
    {
        case 1:
            sound(INTRO4_1);
            break;
        case 12:
            sound(INTRO4_2);
            break;
        case 7:
            sound(INTRO4_3);
            break;
        case 26:
            sound(INTRO4_4);
            break;
    }
}

void intro42animsounds(int32_t fr)
{
    switch(fr)
    {
        case 10:
            sound(INTRO4_6);
            break;
    }
}




void endanimvol41(int32_t fr)
{
    switch(fr)
    {
        case 3:
            sound(DUKE_UNDERWATER);
            break;
        case 35:
            sound(VOL4ENDSND1);
            break;
    }
}

void endanimvol42(int32_t fr)
{
    switch(fr)
    {
        case 11:
            sound(DUKE_UNDERWATER);
            break;
        case 20:
            sound(VOL4ENDSND1);
            break;
        case 39:
            sound(VOL4ENDSND2);
            break;
        case 50:
            FX_StopAllSounds();
            break;
    }
}

void endanimvol43(int32_t fr)
{
    switch(fr)
    {
        case 1:
            sound(BOSS4_DEADSPEECH);
            break;
        case 40:
            sound(VOL4ENDSND1);
            sound(DUKE_UNDERWATER);
            break;
        case 50:
            sound(BIGBANG);
            break;
    }
}


#if DUKE3D_GAMEVER >= GAMEVER_15
int32_t lastanimhack=0;
#endif
/*
================
playanm

Play cached ANM frames with the clip's timing and sound cues,
servicing input and packets until the clip ends or is skipped.
================
*/
void playanm(char *fn,char t)
{
        char *animbuf, *palptr;
    int32_t i, j, k, length=0, numframes=0;
    int32 handle=-1;

    video_flushpending();  // shows frames of its own
//    return;

    if(t != 7 && t != 9 && t != 10 && t != 11)
        KB_FlushKeyboardQueue();

    if( KB_KeyWaiting() )
    {
        FX_StopAllSounds();
        goto ENDOFANIMLOOP;
    }

        handle = kopen4load(fn,0);
        if(handle == -1) return;
        length = kfilelength(handle);

    walock[MAXTILES-3-t] = 219+t;

#if DUKE3D_GAMEVER >= GAMEVER_15
    if(anim == 0 || lastanimhack != (MAXTILES-3-t))
#endif
        allocache((intptr_t *)&anim,length+sizeof(anim_t),&walock[MAXTILES-3-t]);

    animbuf = (char *)(((intptr_t)(anim))+sizeof(anim_t));

#if DUKE3D_GAMEVER >= GAMEVER_15
    lastanimhack = (MAXTILES-3-t);
#endif

    tilesizx[MAXTILES-3-t] = 200;
    tilesizy[MAXTILES-3-t] = 320;

        kread(handle,animbuf,length);
        kclose(handle);

        ANIM_LoadAnim (animbuf);
        numframes = ANIM_NumFrames();

        palptr = ANIM_GetPalette();
        for(i=0;i<256;i++)
        {
                j = (i<<2); k = j-i;
                tempbuf[j+0] = (palptr[k+2]>>2);
                tempbuf[j+1] = (palptr[k+1]>>2);
                tempbuf[j+2] = (palptr[k+0]>>2);
                tempbuf[j+3] = 0;
        }

        VBE_setPalette(0L,256L,tempbuf);
        if (getrendermode() == 3)
        {
            glgamepalette = NULL;  /* curpalette is the ANM's now. */
            setpalettefade(0,0,0,0);  /* VBE_setPalette ends a palto fade in renderer 0. */
        }

    ototalclock = totalclock + 10;

        for(i=1;i<numframes;i++)
        {
       while(totalclock < ototalclock)
       {
          if( KB_KeyWaiting() )
              goto ENDOFANIMLOOP;
          getpackets();
       }

       if(t == 10) ototalclock += 14;
       else if(t == 9) ototalclock += 10;
       else if(t == 7) ototalclock += 18;
       else if(t == 6) ototalclock += 14;
       else if(t == 5) ototalclock += 9;
       else if(ud.volume_number == 3) ototalclock += 10;
       else if(ud.volume_number == 2) ototalclock += 10;
       else if(ud.volume_number == 1) ototalclock += 18;
       else                           ototalclock += 10;

       waloff[MAXTILES-3-t] = ((intptr_t)(ANIM_DrawFrame(i)));
       invalidatetile(MAXTILES-3-t, -1, -1);  /* renderer 3: upload the new frame. */
       if (widescreen) clearallviews(0L);
       rotatesprite(0<<16,0<<16,65536L,512,MAXTILES-3-t,0,0,2+4+8+16+64, 0,0,xdim-1,ydim-1);
       nextpage();

       if(t == 8) endanimvol41(i);
       else if(t == 10) endanimvol42(i);
       else if(t == 11) endanimvol43(i);
       else if(t == 9) intro42animsounds(i);
       else if(t == 7) intro4animsounds(i);
       else if(t == 6) first4animsounds(i);
       else if(t == 5) logoanimsounds(i);
       else if(t < 4) endanimsounds(i);
        }

    ENDOFANIMLOOP:

    ANIM_FreeAnim ();
    walock[MAXTILES-3-t] = 1;
}


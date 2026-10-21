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


void savetemp(char *fn,intptr_t daptr,int32_t dasiz)
{
    int fp;

    fp = open(fn,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

    write(fp,(char *)daptr,dasiz);

    close(fp);
}

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

static int loadpheader(char spot,int32 *vn,int32 *ln,int32 *psk,int32 *nump)
{

     int32_t i;
     char filename[] = "game0.sav";
     char fullname[MAX_PATH];
     int32_t fil;
     int32_t bv = 0;

     filename[4] = spot+'0';
     GetPathFromEnvironment(fullname, sizeof(fullname), filename);

     if ((fil = kopen4load(fullname,0)) == -1) return(-1);

     walock[MAXTILES-3] = 255;

     bv = sg_read32(fil);
     if(bv != BYTEVERSION)
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
     tilesizx[MAXTILES-3] = 100; tilesizy[MAXTILES-3] = 160;
     kdfread((char *)waloff[MAXTILES-3],160,100,fil);

         kclose(fil);

         return(0);
}


int loadplayer(signed char spot)
{
     char fn[] = "game0.sav";
     char mpfn[] = "gameA_00.sav";
     char fullname[MAX_PATH];

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

int loadplayerfile(char *fnptr, signed char spot)
{
     short k,music_changed;
     char scriptptrs[MAXSCRIPTSIZE];
     int32_t fil, bv, i, j, x, animoffs[MAXANIMATES];
     int32 nump;

     if ((fil = kopen4load(fnptr,0)) == -1) return(-1);

     ready2send = 0;

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
#if DUKE3D_GAMEVER < GAMEVER_14
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
     sg_readrecs(fil,wall,sizeof(walltype),sg_wall,MAXWALLS,SG_WALL_SIZE,MAXWALLS);
     numsectors = clampcount(sg_read16(fil),MAXSECTORS);
     sg_readrecs(fil,sector,sizeof(sectortype),sg_sector,MAXSECTORS,SG_SECTOR_SIZE,MAXSECTORS);
     sg_readrecs(fil,sprite,sizeof(spritetype),sg_sprite,MAXSPRITES,SG_SPRITE_SIZE,MAXSPRITES);
     sg_read16s(fil,headspritesect,2,MAXSECTORS+1);
     sg_read16s(fil,prevspritesect,2,MAXSPRITES);
     sg_read16s(fil,nextspritesect,2,MAXSPRITES);
     sg_read16s(fil,headspritestat,2,MAXSTATUS+1);
     sg_read16s(fil,prevspritestat,2,MAXSPRITES);
     sg_read16s(fil,nextspritestat,2,MAXSPRITES);
     numcyclers = clampcount(sg_read16(fil),MAXCYCLERS);
     sg_read16s(fil,&cyclers[0][0],12,MAXCYCLERS);
     sg_readrecs(fil,ps,sizeof(struct player_struct),sg_player,MAXPLAYERS,SG_PLAYER_DFSIZE,SG_PLAYER_DFCOUNT);
     for(i=0;i<MAXPLAYERS;i++) ps[i].palette = (char *) &palette[0];
     sg_readrecs(fil,po,sizeof(struct player_orig),sg_playerorig,MAXPLAYERS,SG_PLAYERORIG_SIZE*MAXPLAYERS,1);
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
     kdfread(&show2dsector[0],sizeof(char),MAXSECTORS>>3,fil);
#if DUKE3D_GAMEVER >= GAMEVER_14 // 1.3D saves no actortype, boardfilename or clouds
     clearbufbyte(actortype,MAXTILES,0L);
     kdfread(&actortype[0],sizeof(char),SG_DOS_MAXTILES,fil);
     kdfread(&boardfilename[0],sizeof(boardfilename),1,fil);

     numclouds = clampcount(sg_read16(fil),128);
     sg_read16s(fil,clouds,sizeof(short)<<7,1);
     sg_read16s(fil,cloudx,sizeof(short)<<7,1);
     sg_read16s(fil,cloudy,sizeof(short)<<7,1);
#endif

     kdfread(&scriptptrs[0],1,MAXSCRIPTSIZE,fil);
     sg_read32s(fil,script,4,MAXSCRIPTSIZE);

     clearbuf(actorscrptr,MAXTILES,0L);
     sg_read32s(fil,actorscrptr,4,SG_DOS_MAXTILES);

#if DUKE3D_GAMEVER < GAMEVER_14
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
     for(i = animatecnt-1;i>=0;i--) animateptr[i] = (int32_t *)((char *)&sector[0]+animoffs[i]);
     sg_read32s(fil,animategoal,4,MAXANIMATES);
     sg_read32s(fil,animatevel,4,MAXANIMATES);

     earthquaketime = sg_read8(fil);
     ud.from_bonus = sg_read16(fil);
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

     resetmys();

     ready2send = 1;

     flushpackets();
     clearfifo();
     waitforeverybody();

     resettimevars();

     return(0);
}

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

int saveplayerfile(char *fnptr, signed char spot)
{
     int32_t i, j, animoffs[MAXANIMATES];
     char scriptptrs[MAXSCRIPTSIZE];
     FILE *fil;

     if ((fil = fopen(fnptr,"wb")) == 0) return(-1);

     ready2send = 0;

     sg_write32(fil,BYTEVERSION);
#if DUKE3D_GAMEVER >= GAMEVER_14 // 1.3D saves have no player count
     sg_write32(fil,ud.multimode);
#endif

     dfwrite(&ud.savegame[spot][0],19,1,fil);
     sg_write32(fil,ud.volume_number);
     sg_write32(fil,ud.level_number);
     sg_write32(fil,ud.player_skill);
     dfwrite((char *)waloff[MAXTILES-1],160,100,fil);

     sg_write16(fil,numwalls);
     sg_writerecs(fil,wall,sizeof(walltype),sg_wall,MAXWALLS,SG_WALL_SIZE,MAXWALLS);
     sg_write16(fil,numsectors);
     sg_writerecs(fil,sector,sizeof(sectortype),sg_sector,MAXSECTORS,SG_SECTOR_SIZE,MAXSECTORS);
     sg_writerecs(fil,sprite,sizeof(spritetype),sg_sprite,MAXSPRITES,SG_SPRITE_SIZE,MAXSPRITES);
     sg_write16s(fil,headspritesect,2,MAXSECTORS+1);
     sg_write16s(fil,prevspritesect,2,MAXSPRITES);
     sg_write16s(fil,nextspritesect,2,MAXSPRITES);
     sg_write16s(fil,headspritestat,2,MAXSTATUS+1);
     sg_write16s(fil,prevspritestat,2,MAXSPRITES);
     sg_write16s(fil,nextspritestat,2,MAXSPRITES);
     sg_write16(fil,numcyclers);
     sg_write16s(fil,&cyclers[0][0],12,MAXCYCLERS);
     sg_writerecs(fil,ps,sizeof(struct player_struct),sg_player,MAXPLAYERS,SG_PLAYER_DFSIZE,SG_PLAYER_DFCOUNT);
     sg_writerecs(fil,po,sizeof(struct player_orig),sg_playerorig,MAXPLAYERS,SG_PLAYERORIG_SIZE*MAXPLAYERS,1);
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
     dfwrite(&show2dsector[0],sizeof(char),MAXSECTORS>>3,fil);
#if DUKE3D_GAMEVER >= GAMEVER_14 // 1.3D saves no actortype, boardfilename or clouds
     dfwrite(&actortype[0],sizeof(char),SG_DOS_MAXTILES,fil);
     dfwrite(&boardfilename[0],sizeof(boardfilename),1,fil);

     sg_write16(fil,numclouds);
     sg_write16s(fil,clouds,sizeof(short)<<7,1);
     sg_write16s(fil,cloudx,sizeof(short)<<7,1);
     sg_write16s(fil,cloudy,sizeof(short)<<7,1);
#endif

     // Script values are already offsets; the pointer flag blocks are kept zeroed.
     clearbufbyte(scriptptrs,MAXSCRIPTSIZE,0L);

     dfwrite(&scriptptrs[0],1,MAXSCRIPTSIZE,fil);
     sg_write32s(fil,script,4,MAXSCRIPTSIZE);

     sg_write32s(fil,actorscrptr,4,SG_DOS_MAXTILES);

#if DUKE3D_GAMEVER < GAMEVER_14
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

static int probe_with_delete(int x,int y,int i,int n,int *hitdelete)
{
    short centre, s;

    s = 1+(CONTROL_GetMouseSensitivity()>>4);

    if( ControllerType == 1 && CONTROL_MousePresent )
    {
        CONTROL_GetInput( &minfo );
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
        if(onbar == 0) return(-probey-2);
        if ( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -128 ) )
            return(probey);
        else if ( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 128 ) )
            return(probey);
        else return(-probey-2);
    }
}

int probe(int x,int y,int i,int n)
{
    return probe_with_delete(x,y,i,n,NULL);
}

int menutext(int x,int y,short s,short p,char *t)
{
    short i, ac, centre;

    y -= 12;

    i = centre = 0;

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


void bar(int x,int y,short *p,short dainc,char damodify,short s, short pa)
{
    short xloc;
    char rev;

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


int32 volnum,levnum,plrskl,numplr;
short lastsavedpos = -1;

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
#if (defined __WATCOMC__)
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

#elif (defined PLATFORM_UNIX)

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
                { "MOUSE AIM", gamefunc_Mouse_Aiming }
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
                { "USE STEROIDS", gamefunc_Steroids }
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
                { "TOGGLE CROSSHAIR", gamefunc_Toggle_Crosshair },
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


#if DUKE3D_GAMEVER < GAMEVER_14
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

void menus(void)
{
    int count;
    short shorttmp = 0;
    short c,x;
    volatile int32_t l;
// CTW - REMOVED
//  int tenerr;
// CTW END - REMOVED

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

    if( (ps[myconnectindex].gm&MODE_MENU) == 0 )
    {
        walock[MAXTILES-3] = 1;
        return;
    }

    ps[myconnectindex].gm &= (0xff-MODE_TYPE);
    ps[myconnectindex].fta = 0;

    x = 0;

    sh = 4-(sintable[(totalclock<<4)&2047]>>11);

    if(!(current_menu >= 1000 && current_menu <= 2999 && current_menu >= 300 && current_menu <= 369))
        vscrn();

    // icculus.org-added menus?
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
            gametext(160,149+16,"PRESS ANY KEY...",0,2+8+16);

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
                if(sprite[ps[myconnectindex].i].extra <= 0)
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
            menutext(160,24,0,0,"ADULT MODE");

            x = probe(60,50+16,16,2);
            if(x == -1) { cmenu(200); break; }

            menutext(c,50+16,SHX(-2),PHX(-2),"ADULT MODE");
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
                            if( wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2 )
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
                            for(x=0;x<numanimwalls;x++)
                            if( wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2 )
                                    if( wall[animwall[x].wallnum].extra >= 0 )
                                        wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
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
                                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                                    break;
                                case FEMPIC2:
                                case FEMPIC3:
                                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
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

            sprintf(tempbuf,"PLAYERS: %-2d                      ",numplr);
            gametext(160,158,tempbuf,0,2+8+16);

            sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+volnum,1+levnum,plrskl);
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
            sprintf(tempbuf,"PLAYERS: %-2d                      ",ud.multimode);
            gametext(160,158,tempbuf,0,2+8+16);

            sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
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
#if DUKE3D_GAMEVER < GAMEVER_14
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
#if DUKE3D_GAMEVER < GAMEVER_14
                if(current_menu > 997) current_menu = 990;
#else
                if(current_menu > 992) current_menu = 990;
#endif
            }

#if DUKE3D_GAMEVER < GAMEVER_14
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

        case 0:
            c = (320>>1);
            rotatesprite(c<<16,(28<<16)-5,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
#if DUKE3D_GAMEVER >= GAMEVER_14 // no Plutonium Pak logo in 1.3D's menus
            rotatesprite((c+100)<<16,(36<<16)-5,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
#endif
// CTW - MODIFICATION
//          x = probe(c,67-3,16,7);
            count = 6;
#if NET_MENU
            count++;
#endif
            x = probe(c,67-3,16,count);
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
#if NET_MENU
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
                    menutext(c,67-3,SHX(-2),PHX(-2),"NEW GAME");
                else
                    menutext(c,67-3,SHX(-2),1,"NEW GAME");
            }
            else
                menutext(c,67-3,SHX(-2),PHX(-2),"NEW GAME");

// CTW - REMOVED
/*          if(movesperpacket != 4 && numplayers < 2)
                menutext(c,67-3+16,SHX(-3),PHX(-3),"PLAY ON TEN");
            else
                menutext(c,67-3+16,SHX(-3),1,"PLAY ON TEN");*/
// CTW END - REMOVED

// CTW - MODIFICATION - Not going to comment out the exact old code here.
// I shifted up every menu option by 16.
// I shifted up every menu result by 1.
            menutext(c,67-3+16,SHX(-3),PHX(-3),"OPTIONS");

            if(movesperpacket == 4 && connecthead != myconnectindex)
                menutext(c,67-3+16+16,SHX(-4),1,"LOAD GAME");
            else menutext(c,67-3+16+16,SHX(-4),PHX(-4),"LOAD GAME");

#ifndef VOLUMEALL
            menutext(c,67-3+16+16+16,SHX(-5),PHX(-5),"HOW TO ORDER");
#else
            menutext(c,67-3+16+16+16,SHX(-5),PHX(-5),"HELP");
#endif
            menutext(c,67-3+16+16+16+16,SHX(-6),PHX(-6),"CREDITS");

#if NET_MENU
            menutext(c,67-3+16+16+16+16+16,SHX(-7),PHX(-7),"NETWORK GAME");
            menutext(c,67-3+16+16+16+16+16+16,SHX(-8),PHX(-8),"QUIT");
#else
            menutext(c,67-3+16+16+16+16+16,SHX(-7),PHX(-7),"QUIT");
#endif

            break;
// CTW END - MODIFICATION

        case 50:
            c = (320>>1);
            rotatesprite(c<<16,32<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
#if DUKE3D_GAMEVER >= GAMEVER_14 // no Plutonium Pak logo in 1.3D's menus
            rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
#endif
            x = probe(c,67,16,7);
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
                menutext(c,67                  ,SHX(-2),1,"NEW GAME");
                menutext(c,67+16               ,SHX(-3),1,"SAVE GAME");
                menutext(c,67+16+16            ,SHX(-4),1,"LOAD GAME");
            }
            else
            {
                menutext(c,67                  ,SHX(-2),PHX(-2),"NEW GAME");
                menutext(c,67+16               ,SHX(-3),PHX(-3),"SAVE GAME");
                menutext(c,67+16+16            ,SHX(-4),PHX(-4),"LOAD GAME");
            }

            menutext(c,67+16+16+16         ,SHX(-5),PHX(-5),"OPTIONS");
#ifndef VOLUMEALL
            menutext(c,67+16+16+16+16      ,SHX(-6),PHX(-6),"HOW TO ORDER");
#else
            menutext(c,67+16+16+16+16      ,SHX(-6),PHX(-6)," HELP");
#endif
            if(numplayers > 1)
                menutext(c,67+16+16+16+16+16   ,SHX(-7),1,"QUIT TO TITLE");
            else menutext(c,67+16+16+16+16+16   ,SHX(-7),PHX(-7),"QUIT TO TITLE");
            menutext(c,67+16+16+16+16+16+16,SHX(-8),PHX(-8),"QUIT GAME");
            break;

        case 100:
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SELECT AN EPISODE");
#ifdef PLUTOPAK
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

        case 200:

            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"OPTIONS");

            c = (320>>1)-120;

            onbar = (probey == 3 || probey == 4 || probey == 5);
            count = 10;
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
                case 6:
                    if ( ControllerType == controltype_keyboardandmouse )
                        ud.mouseflip = 1-ud.mouseflip;
                    break;
                case 7:
                    cmenu(700);
                    break;
                case 8:
#ifndef AUSTRALIA
                    cmenu(10000);
#endif
                    break;
                case 9:
                    if( (ps[myconnectindex].gm&MODE_GAME) )
                    {
                        closedemowrite();
                        break;
                    }
                    ud.m_recstat = !ud.m_recstat;
                    break;
#if CONTROLS_CONFIG_MENU
                case 10:
                    cmenu(MENU_CONTROLS);
                    break;
#endif
                default:
                    if(x == count-1) cmenu(710);
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

            menutext(c,31+15+15+15+15,SHX(-6),PHX(-6),"BRIGHTNESS");
                shorttmp = (short) ud.brightness;
                bar(c+167+40,31+15+15+15+15,(short *)&shorttmp,8,x==4,SHX(-6),PHX(-6));
                ud.brightness = (int32) shorttmp;
                if(x==4) setbrightness(ud.brightness>>2,&ps[myconnectindex].palette[0]);

            if ( ControllerType == controltype_keyboardandmouse )
            {
                short sense;
                sense = CONTROL_GetMouseSensitivity()>>10;

                menutext(c,31+15+15+15+15+15,SHX(-7),PHX(-7),"MOUSE SENSITIVITY");
                bar(c+167+40,31+15+15+15+15+15,&sense,4,x==5,SHX(-7),PHX(-7));
                CONTROL_SetMouseSensitivity( sense<<10 );
                menutext(c,31+15+15+15+15+15+15,SHX(-7),PHX(-7),"MOUSE AIMING FLIP");

                if(ud.mouseflip) menutext(c+160+40,31+15+15+15+15+15+15,SHX(-7),PHX(-7),"ON");
                else menutext(c+160+40,31+15+15+15+15+15+15,SHX(-7),PHX(-7),"OFF");

            }
            else
            {
                short sense;
                sense = CONTROL_GetMouseSensitivity()>>10;

                menutext(c,31+15+15+15+15+15,SHX(-7),1,"MOUSE SENSITIVITY");
                bar(c+167+40,31+15+15+15+15+15,&sense,4,x==99,SHX(-7),1);
                menutext(c,31+15+15+15+15+15+15,SHX(-7),1,"MOUSE AIMING FLIP");

                if(ud.mouseflip) menutext(c+160+40,31+15+15+15+15+15+15,SHX(-7),1,"ON");
                else menutext(c+160+40,31+15+15+15+15+15+15,SHX(-7),1,"OFF");
            }

            menutext(c,31+15+15+15+15+15+15+15,SHX(-8),PHX(-8),"SOUNDS");
#ifndef AUSTRALIA
            menutext(c,31+15+15+15+15+15+15+15+15,SHX(-9),PHX(-9),"PARENTAL LOCK");
#else
            menutext(c,31+15+15+15+15+15+15+15+15,SHX(-9),1,"PARENTAL LOCK");
#endif
            if( (ps[myconnectindex].gm&MODE_GAME) && ud.m_recstat != 1 )
            {
                menutext(c,31+15+15+15+15+15+15+15+15+15,SHX(-10),1,"RECORD");
                menutext(c+160+40,31+15+15+15+15+15+15+15+15+15,SHX(-10),1,"OFF");
            }
            else
            {
                menutext(c,31+15+15+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"RECORD");

                if(ud.m_recstat == 1)
                    menutext(c+160+40,31+15+15+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"ON");
                else menutext(c+160+40,31+15+15+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"OFF");
            }

#if CONTROLS_CONFIG_MENU
            menutext(c,31+15+15+15+15+15+15+15+15+15+15,SHX(-11),PHX(-11),"CONTROLS...");
#endif
            menutext(c,31+15*(count-1),SHX(-12),PHX(-12),"VIDEO...");
            break;

        case 710:
            // VIDEO: the look of renderer 3's GL 2.1 shader path (greyed
            // elsewhere), then the display mode and vsync, which change at
            // once. Rows from y 50, 16 apart, as SOUNDS.
            {
                int32 shader = 0;
                short was;
#ifdef USE_OPENGL
                shader = (getrendermode() == 3) && (gldrawpath == GLPATH_SHADER);
#endif
                c = (320>>1)-120;
                rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
                menutext(320>>1,24,0,0,"VIDEO");
                onbar = ( probey == 0 || probey == 1 );

                x = probe(c,50,16,4);
                if(x == -1) cmenu(200);
                else if(x == 2)
                {
                    DisplayMode = (DisplayMode+1)%DISPLAY_COUNT;
                    setvideodisplaymode(DisplayMode);
                }
                else if(x == 3)
                {
                    VSync = !VSync;
                    setvideovsync(VSync);
                }

                // Slider positions 0..60, 1.0 at 30: contrast 0.5..1.5 in
                // steps of 0.1, saturation 0..2 in steps of 0.1. The setting
                // changes only when its slider moves.
                menutext(c,50,SHX(-2),!shader,"CONTRAST");
                was = shorttmp = (short) ((GLContrast-GL_CONTRAST_MIN)*60.0+0.5);
                bar(c+167+40,50,(short *)&shorttmp,6,shader && x==0,SHX(-2),!shader);
                if(shorttmp != was)
                    GLContrast = GL_CONTRAST_MIN+min(shorttmp,60)/60.0;

                menutext(c,50+16,SHX(-3),!shader,"SATURATION");
                was = shorttmp = (short) (GLSaturation*30.0+0.5);
                bar(c+167+40,50+16,(short *)&shorttmp,3,shader && x==1,SHX(-3),!shader);
                if(shorttmp != was)
                    GLSaturation = min(shorttmp,60)/30.0;

#ifdef USE_OPENGL
                glcontrast = GLContrast;
                glsaturation = GLSaturation;
#endif

                menutext(c,50+16+16,SHX(-4),PHX(-4),"DISPLAY");
                switch(DisplayMode)
                {
                    case DISPLAY_WINDOWED: menutext(c+110,50+16+16,0,0,"WINDOWED");break;
                    case DISPLAY_BORDERLESS: menutext(c+110,50+16+16,0,0,"BORDERLESS");break;
                    default: menutext(c+110,50+16+16,0,0,"EXCLUSIVE");break;
                }

                menutext(c,50+16+16+16,SHX(-5),PHX(-5),"VSYNC");
                menutext(c+110,50+16+16+16,0,0,VSync ? "ON" : "OFF");
                if(!shader)
                {
                    gametext(160,170,"CONTRAST AND SATURATION NEED",0,2+8+16);
                    gametext(160,170+9,"THE OPENGL 2.1 SHADER PATH",0,2+8+16);
                }
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

            if(current_menu >= 360 && current_menu <= 369 )
            {
                sprintf(tempbuf,"PLAYERS: %-2d                      ",ud.multimode);
                gametext(160,158,tempbuf,0,2+8+16);
                sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
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
                  sprintf(tempbuf,"PLAYERS: %-2d                      ",numplr);
                  gametext(160,158,tempbuf,0,2+8+16);
                  sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+volnum,1+levnum,plrskl);
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
              sprintf(tempbuf,"PLAYERS: %-2d                      ",ud.multimode);
              gametext(160,158,tempbuf,0,2+8+16);
              sprintf(tempbuf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
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
                if(ud.recstat == 1)
                    closedemowrite();
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
    char *pal = ps[((ps[myconnectindex].gm&(MODE_GAME|MODE_DEMO)) &&
                    !(ps[myconnectindex].gm&MODE_EOL)) ? screenpeek : myconnectindex].palette;
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

void palto(char r,char g,char b,int32_t e)
{
    int i;
    char temparray[768];

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
void playanm(char *fn,char t)
{
        char *animbuf, *palptr;
    int32_t i, j, k, length=0, numframes=0;
    int32 handle=-1;

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

    animbuf = (char *)(FP_OFF(anim)+sizeof(anim_t));

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

       waloff[MAXTILES-3-t] = FP_OFF(ANIM_DrawFrame(i));
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


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

// PRIMITIVE


char haltsoundhack;
short callsound(short sn,short whatsprite)
{
    short i;

    if(haltsoundhack)
    {
        haltsoundhack = 0;
        return -1;
    }

    i = headspritesect[sn];
    while(i >= 0)
    {
        if( PN == MUSICANDSFX && SLT < 1000 )
        {
            if(whatsprite == -1) whatsprite = i;

            if(T1 == 0)
            {
                if( (soundm[SLT]&16) == 0)
                {
                    if(SLT)
                    {
                        spritesound(SLT,whatsprite);
                        if(SHT && SLT != SHT && SHT < GAME_NUM_SOUNDS)
                            stopsound(SHT);
                    }

                    if( (sector[SECT].lotag&0xff) != 22)
                        T1 = 1;
                }
            }
            else if(SHT < GAME_NUM_SOUNDS)
            {
                if(SHT) spritesound(SHT,whatsprite);
                if( (soundm[SLT]&1) || ( SHT && SHT != SLT ) )
                    stopsound(SLT);
                T1 = 0;
            }
#if DUKE3D_GAMEVER < GAMEVER_14
            return sn;
#else
            return SLT;
#endif
        }
        i = nextspritesect[i];
    }
    return -1;
}


short check_activator_motion( short lotag )
{
    short i, j;
    spritetype *s;

    i = headspritestat[8];
    while ( i >= 0 )
    {
        if ( sprite[i].lotag == lotag )
        {
            s = &sprite[i];

            for ( j = animatecnt-1; j >= 0; j-- )
                if ( s->sectnum == animatesect[j] )
                    return( 1 );

            j = headspritestat[3];
            while ( j >= 0 )
            {
                if(s->sectnum == sprite[j].sectnum)
                    switch(sprite[j].lotag)
                    {
                        case 11:
                        case 30:
                            if ( hittype[j].temp_data[4] )
                                return( 1 );
                            break;
                        case 20:
                        case 31:
#if DUKE3D_GAMEVER < GAMEVER_14
                        case 36:
                        case 32:
#else
                        case 32:
                        case 18:
#endif
                            if ( hittype[j].temp_data[0] )
                                return( 1 );
                            break;
                    }

                j = nextspritestat[j];
            }
        }
        i = nextspritestat[i];
    }
    return( 0 );
}

char isadoorwall(short dapic)
{
    switch(dapic)
    {
        case DOORTILE1:
        case DOORTILE2:
        case DOORTILE3:
        case DOORTILE4:
        case DOORTILE5:
        case DOORTILE6:
        case DOORTILE7:
        case DOORTILE8:
        case DOORTILE9:
        case DOORTILE10:
        case DOORTILE11:
        case DOORTILE12:
        case DOORTILE14:
        case DOORTILE15:
        case DOORTILE16:
        case DOORTILE17:
        case DOORTILE18:
        case DOORTILE19:
        case DOORTILE20:
        case DOORTILE21:
        case DOORTILE22:
#if DUKE3D_GAMEVER >= GAMEVER_14
        case DOORTILE23:
#endif
            return 1;
    }
    return 0;
}


char isanunderoperator(short lotag)
{
    switch(lotag&0xff)
    {
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
#if DUKE3D_GAMEVER >= GAMEVER_14
        case 22:
#endif
        case 26:
            return 1;
    }
    return 0;
}

char isanearoperator(short lotag)
{
    switch(lotag&0xff)
    {
        case 9:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 25:
        case 26:
        case 29://Toothed door
            return 1;
    }
    return 0;
}

short checkcursectnums(short sect)
{
    short i;
    for(i=connecthead;i>=0;i=connectpoint2[i])
        if( sprite[ps[i].i].sectnum == sect ) return i;
    return -1;
}

int32_t ldist(spritetype *s1,spritetype *s2)
{
    int32_t vx,vy;
    vx = s1->x - s2->x;
    vy = s1->y - s2->y;
#if DUKE3D_GAMEVER < GAMEVER_14
    return(FindDistance2D(vx,vy));
#else
    return(FindDistance2D(vx,vy) + 1);
#endif
}

int32_t dist(spritetype *s1,spritetype *s2)
{
    int32_t vx,vy,vz;
    vx = s1->x - s2->x;
    vy = s1->y - s2->y;
    vz = s1->z - s2->z;
    return(FindDistance3D(vx,vy,vz>>4));
}

short findplayer(spritetype *s,int32_t *d)
{
    short j, closest_player;
    int32_t x, closest;

    if(ud.multimode < 2)
    {
        *d = klabs(ps[myconnectindex].oposx-s->x) + klabs(ps[myconnectindex].oposy-s->y) + ((klabs(ps[myconnectindex].oposz-s->z+(28<<8)))>>4);
        return myconnectindex;
    }

    closest = 0x7fffffff;
    closest_player = 0;

    for(j=connecthead;j>=0;j=connectpoint2[j])
    {
        x = klabs(ps[j].oposx-s->x) + klabs(ps[j].oposy-s->y) + ((klabs(ps[j].oposz-s->z+(28<<8)))>>4);
        if( x < closest && sprite[ps[j].i].extra > 0 )
        {
            closest_player = j;
            closest = x;
        }
    }

    *d = closest;
    return closest_player;
}

short findotherplayer(short p,int32_t *d)
{
    short j, closest_player;
    int32_t x, closest;

    closest = 0x7fffffff;
    closest_player = p;

    for(j=connecthead;j>=0;j=connectpoint2[j])
        if(p != j && sprite[ps[j].i].extra > 0)
    {
        x = klabs(ps[j].oposx-ps[p].posx) + klabs(ps[j].oposy-ps[p].posy) + (klabs(ps[j].oposz-ps[p].posz)>>4);

        if( x < closest )
        {
            closest_player = j;
            closest = x;
        }
    }

    *d = closest;
    return closest_player;
}



void doanimations(void)
{
	int32_t i, j, a, p, v, dasect;

	for(i=animatecnt-1;i>=0;i--)
	{
		a = *animateptr[i];
		v = animatevel[i]*TICSPERFRAME;
		dasect = animatesect[i];

        if (a == animategoal[i])
        {
            stopinterpolation(animateptr[i]);

            animatecnt--;
            animateptr[i] = animateptr[animatecnt];
            animategoal[i] = animategoal[animatecnt];
            animatevel[i] = animatevel[animatecnt];
            animatesect[i] = animatesect[animatecnt];
            if( sector[animatesect[i]].lotag == 18 || sector[animatesect[i]].lotag == 19 )
                if(animateptr[i] == &sector[animatesect[i]].ceilingz)
                    continue;

           if( (sector[dasect].lotag&0xff) != 22 )
                callsound(dasect,-1);

            continue;
        }

        if (v > 0) { a = min(a+v,animategoal[i]); }
              else { a = max(a+v,animategoal[i]); }

        if( animateptr[i] == &sector[animatesect[i]].floorz)
        {
            for(p=connecthead;p>=0;p=connectpoint2[p])
                if (ps[p].cursectnum == dasect)
                    if ((sector[dasect].floorz-ps[p].posz) < (64<<8))
                        if (sprite[ps[p].i].owner >= 0)
            {
                ps[p].posz += v;
                ps[p].poszv = 0;
                if (p == myconnectindex)
                {
                    myz += v;
                    myzvel = 0;
                    myzbak[((movefifoplc-1)&(MOVEFIFOSIZ-1))] = ps[p].posz;
                }
            }

            for(j=headspritesect[dasect];j>=0;j=nextspritesect[j])
                if (sprite[j].statnum != 3)
                {
                    hittype[j].bposz = sprite[j].z;
                    sprite[j].z += v;
                    hittype[j].floorz = sector[dasect].floorz+v;
                }
        }

		*animateptr[i] = a;
	}
}

int32_t getanimationgoal(int32_t *animptr)
{
	int32_t i, j;

	j = -1;
    for(i=animatecnt-1;i>=0;i--)
        if (animptr == (int32_t *)animateptr[i])
		{
			j = i;
			break;
		}
	return(j);
}

static int32_t setanimation(short animsect,int32_t *animptr, int32_t thegoal, int32_t thevel)
{
	int32_t i, j;

	if (animatecnt >= MAXANIMATES-1)
		return(-1);

	j = animatecnt;
    for(i=0;i<animatecnt;i++)
		if (animptr == animateptr[i])
		{
			j = i;
			break;
		}

    animatesect[j] = animsect;
	animateptr[j] = animptr;
	animategoal[j] = thegoal;
    if (thegoal >= *animptr)
       animatevel[j] = thevel;
    else
       animatevel[j] = -thevel;
    
    if (j == animatecnt) animatecnt++;

    setinterpolation(animptr);

    return(j);
}




void animatecamsprite(void)
{
    short i;

    if(camsprite <= 0) return;

    i = camsprite;

    if(T1 >= 11)
    {
        T1 = 0;

        if(ps[screenpeek].newowner >= 0)
            OW = ps[screenpeek].newowner;

        else if(OW >= 0 && dist(&sprite[ps[screenpeek].i],&sprite[i]) < 2048)
            xyzmirror(OW,PN);
    }
    else T1++;
}

void animatewalls(void)
{
    int32_t i, j, p, t;

#if DUKE3D_GAMEVER < GAMEVER_13D
    for(p=numanimwalls-1;p>=0;p--)
#else
    for(p=0;p < numanimwalls ;p++)
//    for(p=numanimwalls-1;p>=0;p--)
#endif
    {
        i = animwall[p].wallnum;
        j = wall[i].picnum;

            switch(j)
            {
                case SCREENBREAK1:
                case SCREENBREAK2:
                case SCREENBREAK3:
                case SCREENBREAK4:
                case SCREENBREAK5:

                case SCREENBREAK9:
                case SCREENBREAK10:
                case SCREENBREAK11:
                case SCREENBREAK12:
                case SCREENBREAK13:
#if DUKE3D_GAMEVER >= GAMEVER_14
                case SCREENBREAK14:
                case SCREENBREAK15:
                case SCREENBREAK16:
                case SCREENBREAK17:
                case SCREENBREAK18:
                case SCREENBREAK19:
#endif

                    if( (TRAND&255) < 16)
                    {
                        animwall[p].tag = wall[i].picnum;
                        wall[i].picnum = SCREENBREAK6;
                    }

                    continue;

                case SCREENBREAK6:
                case SCREENBREAK7:
                case SCREENBREAK8:

                    if(animwall[p].tag >= 0 && wall[i].extra != FEMPIC2 && wall[i].extra != FEMPIC3 )
                        wall[i].picnum = animwall[p].tag;
                    else
                    {
                        wall[i].picnum++;
                        if(wall[i].picnum == (SCREENBREAK6+3) )
                            wall[i].picnum = SCREENBREAK6;
                    }
                    continue;

            }

        if(wall[i].cstat&16)
            switch(wall[i].overpicnum)
        {
            case W_FORCEFIELD:
            case W_FORCEFIELD+1:
            case W_FORCEFIELD+2:

                t = animwall[p].tag;

                if(wall[i].cstat&254)
                {
                    wall[i].xpanning -= t>>10; // sintable[(t+512)&2047]>>12;
                    wall[i].ypanning -= t>>10; // sintable[t&2047]>>12;

                    if(wall[i].extra == 1)
                    {
                        wall[i].extra = 0;
                        animwall[p].tag = 0;
                    }
                    else
                        animwall[p].tag+=128;

                    if( animwall[p].tag < (128<<4) )
                    {
                        if( animwall[p].tag&128 )
                            wall[i].overpicnum = W_FORCEFIELD;
                        else wall[i].overpicnum = W_FORCEFIELD+1;
                    }
                    else
                    {
                        if( (TRAND&255) < 32 )
                            animwall[p].tag = 128<<(TRAND&3);
                        else wall[i].overpicnum = W_FORCEFIELD+1;
                    }
                }

                break;
        }
    }
}

char activatewarpelevators(short s,short d) //Parm = sectoreffectornum
{
    short i, sn;

    sn = sprite[s].sectnum;

    // See if the sector exists

    i = headspritestat[3];
    while(i >= 0)
    {
        if( SLT == 17 )
            if( SHT == sprite[s].hitag )
                if( (klabs(sector[sn].floorz-hittype[s].temp_data[2]) > SP) ||
                    (sector[SECT].hitag == (sector[sn].hitag-d) ) )
                        break;
        i = nextspritestat[i];
    }

    if(i==-1)
    {
        d = 0;
        return 1; // No find
    }
    else
    {
        if(d == 0)
            spritesound(ELEVATOR_OFF,s);
        else spritesound(ELEVATOR_ON,s);
    }


    i = headspritestat[3];
    while(i >= 0)
    {
        if( SLT == 17 )
            if( SHT == sprite[s].hitag )
            {
                T1 = d;
                T2 = d; //Make all check warp
            }
        i = nextspritestat[i];
    }
    return 0;
}



void operatesectors(short sn,short ii)
{
    int32_t j=0l, l=0l, q=0l, startwall=0l, endwall=0l;
    short i=0;
    char sect_error;
    sectortype *sptr;

    sect_error = 0;
    sptr = &sector[sn];

    switch(sptr->lotag&(0xffff-49152))
    {

        case 30:
            j = sector[sn].hitag;
            if( hittype[j].tempang == 0 ||
                hittype[j].tempang == 256)
                    callsound(sn,ii);
            if(sprite[j].extra == 1)
                sprite[j].extra = 3;
            else sprite[j].extra = 1;
            break;

        case 31:

            j = sector[sn].hitag;
            if(hittype[j].temp_data[4] == 0)
                hittype[j].temp_data[4] = 1;
            
            callsound(sn,ii);
            break;

        case 26: //The split doors
            i = getanimationgoal(&sptr->ceilingz);
            if(i == -1) //if the door has stopped
            {
                haltsoundhack = 1;
                sptr->lotag &= 0xff00;
                sptr->lotag |= 22;
                operatesectors(sn,ii);
                sptr->lotag &= 0xff00;
                sptr->lotag |= 9;
                operatesectors(sn,ii);
                sptr->lotag &= 0xff00;
                sptr->lotag |= 26;
            }
            return;

        case 9:
        {
            int32_t dax,day,dax2,day2,sp;
            int32_t wallfind[2];

            startwall = sptr->wallptr;
            endwall = startwall+sptr->wallnum-1;

            sp = sptr->extra>>4;

            //first find center point by averaging all points
            dax = 0L, day = 0L;
            for(i=startwall;i<=endwall;i++)
            {
                dax += wall[i].x;
                day += wall[i].y;
            }
            dax /= (endwall-startwall+1);
            day /= (endwall-startwall+1);

            //find any points with either same x or same y coordinate
            //  as center (dax, day) - should be 2 points found.
            wallfind[0] = -1;
            wallfind[1] = -1;
            for(i=startwall;i<=endwall;i++)
                if ((wall[i].x == dax) || (wall[i].y == day))
                {
                    if (wallfind[0] == -1)
                        wallfind[0] = i;
                    else wallfind[1] = i;
                }

            for(j=0;j<2;j++)
            {
                if ((wall[wallfind[j]].x == dax) && (wall[wallfind[j]].y == day))
                {
                    //find what direction door should open by averaging the
                    //  2 neighboring points of wallfind[0] & wallfind[1].
                    i = wallfind[j]-1; if (i < startwall) i = endwall;
                    dax2 = ((wall[i].x+wall[wall[wallfind[j]].point2].x)>>1)-wall[wallfind[j]].x;
                    day2 = ((wall[i].y+wall[wall[wallfind[j]].point2].y)>>1)-wall[wallfind[j]].y;
                    if (dax2 != 0)
                    {
                        dax2 = wall[wall[wall[wallfind[j]].point2].point2].x;
                        dax2 -= wall[wall[wallfind[j]].point2].x;
                        setanimation(sn,&wall[wallfind[j]].x,wall[wallfind[j]].x+dax2,sp);
                        setanimation(sn,&wall[i].x,wall[i].x+dax2,sp);
                        setanimation(sn,&wall[wall[wallfind[j]].point2].x,wall[wall[wallfind[j]].point2].x+dax2,sp);
                        callsound(sn,ii);
                    }
                    else if (day2 != 0)
                    {
                        day2 = wall[wall[wall[wallfind[j]].point2].point2].y;
                        day2 -= wall[wall[wallfind[j]].point2].y;
                        setanimation(sn,&wall[wallfind[j]].y,wall[wallfind[j]].y+day2,sp);
                        setanimation(sn,&wall[i].y,wall[i].y+day2,sp);
                        setanimation(sn,&wall[wall[wallfind[j]].point2].y,wall[wall[wallfind[j]].point2].y+day2,sp);
                        callsound(sn,ii);
                    }
                }
                else
                {
                    i = wallfind[j]-1; if (i < startwall) i = endwall;
                    dax2 = ((wall[i].x+wall[wall[wallfind[j]].point2].x)>>1)-wall[wallfind[j]].x;
                    day2 = ((wall[i].y+wall[wall[wallfind[j]].point2].y)>>1)-wall[wallfind[j]].y;
                    if (dax2 != 0)
                    {
                        setanimation(sn,&wall[wallfind[j]].x,dax,sp);
                        setanimation(sn,&wall[i].x,dax+dax2,sp);
                        setanimation(sn,&wall[wall[wallfind[j]].point2].x,dax+dax2,sp);
                        callsound(sn,ii);
                    }
                    else if (day2 != 0)
                    {
                        setanimation(sn,&wall[wallfind[j]].y,day,sp);
                        setanimation(sn,&wall[i].y,day+day2,sp);
                        setanimation(sn,&wall[wall[wallfind[j]].point2].y,day+day2,sp);
                        callsound(sn,ii);
                    }
                }
            }

        }
        return;

        case 15://Warping elevators

            if(sprite[ii].picnum != APLAYER) return;
//            if(ps[sprite[ii].yvel].select_dir == 1) return;

            i = headspritesect[sn];
            while(i >= 0)
            {
                if(PN==SECTOREFFECTOR && SLT == 17 ) break;
                i = nextspritesect[i];
            }

            if(sprite[ii].sectnum == sn)
            {
                if( activatewarpelevators(i,-1) )
                    activatewarpelevators(i,1);
                else if( activatewarpelevators(i,1) )
                    activatewarpelevators(i,-1);
                return;
            }
            else
            {
                if(sptr->floorz > SZ)
                    activatewarpelevators(i,-1);
                else
                    activatewarpelevators(i,1);
            }

            return;

        case 16:
        case 17:

            i = getanimationgoal(&sptr->floorz);

            if(i == -1)
            {
                i = nextsectorneighborz(sn,sptr->floorz,1,1);
                if( i == -1 )
                {
                    i = nextsectorneighborz(sn,sptr->floorz,1,-1);
                    if( i == -1 ) return;
                    j = sector[i].floorz;
                    setanimation(sn,&sptr->floorz,j,sptr->extra);
                }
                else
                {
                    j = sector[i].floorz;
                    setanimation(sn,&sptr->floorz,j,sptr->extra);
                }
                callsound(sn,ii);
            }

            return;

        case 18:
        case 19:

            i = getanimationgoal(&sptr->floorz);

            if(i==-1)
            {
                i = nextsectorneighborz(sn,sptr->floorz,1,-1);
                if(i==-1) i = nextsectorneighborz(sn,sptr->floorz,1,1);
                if(i==-1) return;
                j = sector[i].floorz;
                q = sptr->extra;
                l = sptr->ceilingz-sptr->floorz;
                setanimation(sn,&sptr->floorz,j,q);
                setanimation(sn,&sptr->ceilingz,j+l,q);
                callsound(sn,ii);
            }
            return;

        case 29:

            if(sptr->lotag&0x8000)
                j = sector[nextsectorneighborz(sn,sptr->ceilingz,1,1)].floorz;
            else
                j = sector[nextsectorneighborz(sn,sptr->ceilingz,-1,-1)].ceilingz;

            i = headspritestat[3]; //Effectors
            while(i >= 0)
            {
                if( (SLT == 22) &&
                    (SHT == sptr->hitag) )
                {
                    sector[SECT].extra = -sector[SECT].extra;

                    T1 = sn;
                    T2 = 1;
                }
                i = nextspritestat[i];
            }

            sptr->lotag ^= 0x8000;

            setanimation(sn,&sptr->ceilingz,j,sptr->extra);

            callsound(sn,ii);

            return;

        case 20:

            REDODOOR:

            if(sptr->lotag&0x8000)
            {
                i = headspritesect[sn];
                while(i >= 0)
                {
                    if(sprite[i].statnum == 3 && SLT==9)
                    {
                        j = SZ;
                        break;
                    }
                    i = nextspritesect[i];
                }
                if(i==-1)
                    j = sptr->floorz;
            }
            else
            {
                j = nextsectorneighborz(sn,sptr->ceilingz,-1,-1);

                if(j >= 0) j = sector[j].ceilingz;
                else
                {
                    sptr->lotag |= 32768;
                    goto REDODOOR;
                }
            }

#if DUKE3D_GAMEVER < GAMEVER_14
            // 1.3D sets the bit from the door's position instead of toggling it.
            if(sptr->ceilingz == sptr->floorz || j == sptr->ceilingz)
                sptr->lotag |= 0x8000;
            else sptr->lotag &= ~0x8000;
#else
            sptr->lotag ^= 0x8000;
#endif

            setanimation(sn,&sptr->ceilingz,j,sptr->extra);
            callsound(sn,ii);

            return;

        case 21:
            i = getanimationgoal(&sptr->floorz);
            if (i >= 0)
            {
                if (animategoal[sn] == sptr->ceilingz)
                    animategoal[i] = sector[nextsectorneighborz(sn,sptr->ceilingz,1,1)].floorz;
                else animategoal[i] = sptr->ceilingz;
                j = animategoal[i];
            }
            else
            {
                if (sptr->ceilingz == sptr->floorz)
                    j = sector[nextsectorneighborz(sn,sptr->ceilingz,1,1)].floorz;
                else j = sptr->ceilingz;

                sptr->lotag ^= 0x8000;

                if(setanimation(sn,&sptr->floorz,j,sptr->extra) >= 0)
                    callsound(sn,ii);
            }
            return;

        case 22:

            // REDODOOR22:

            if ( (sptr->lotag&0x8000) )
            {
                q = (sptr->ceilingz+sptr->floorz)>>1;
                j = setanimation(sn,&sptr->floorz,q,sptr->extra);
                j = setanimation(sn,&sptr->ceilingz,q,sptr->extra);
            }
            else
            {
                q = sector[nextsectorneighborz(sn,sptr->floorz,1,1)].floorz;
                j = setanimation(sn,&sptr->floorz,q,sptr->extra);
                q = sector[nextsectorneighborz(sn,sptr->ceilingz,-1,-1)].ceilingz;
                j = setanimation(sn,&sptr->ceilingz,q,sptr->extra);
            }

            sptr->lotag ^= 0x8000;

#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 opens split doors silently
            callsound(sn,ii);
#endif

            return;

        case 23: //Swingdoor

            j = -1;
            q = 0;

            i = headspritestat[3];
            while(i >= 0)
            {
                if( SLT == 11 && SECT == sn && !T5)
                {
                    j = i;
                    break;
                }
                i = nextspritestat[i];
            }

            l = sector[SECT].lotag&0x8000;

            if(j >= 0)
            {
                i = headspritestat[3];
                while(i >= 0)
                {
                    if( l == (sector[SECT].lotag&0x8000) && SLT == 11 && sprite[j].hitag == SHT && !T5 )
                    {
                        if(sector[SECT].lotag&0x8000) sector[SECT].lotag &= 0x7fff;
                        else sector[SECT].lotag |= 0x8000;
                        T5 = 1;
                        T4 = -T4;
                        if(q == 0)
                        {
                            callsound(sn,i);
                            q = 1;
                        }
                    }
                    i = nextspritestat[i];
                }
            }
            return;

        case 25: //Subway type sliding doors

            j = headspritestat[3];
            while(j >= 0)//Find the sprite
            {
                if( (sprite[j].lotag) == 15 && sprite[j].sectnum == sn )
                    break; //Found the sectoreffector.
                j = nextspritestat[j];
            }

            if(j < 0)
                return;

            i = headspritestat[3];
            while(i >= 0)
            {
                if( SHT==sprite[j].hitag )
                {
                    if( SLT == 15 )
                    {
                        sector[SECT].lotag ^= 0x8000; // Toggle the open or close
                        SA += 1024;
                        if(T5) callsound(SECT,i);
                        callsound(SECT,i);
                        if(sector[SECT].lotag&0x8000) T5 = 1;
                        else T5 = 2;
                    }
                }
                i = nextspritestat[i];
            }
            return;

        case 27:  //Extended bridge

            j = headspritestat[3];
            while(j >= 0)
            {
                if( (sprite[j].lotag&0xff)==20 && sprite[j].sectnum == sn) //Bridge
                {

                    sector[sn].lotag ^= 0x8000;
                    if(sector[sn].lotag&0x8000) //OPENING
                        hittype[j].temp_data[0] = 1;
                    else hittype[j].temp_data[0] = 2;
                    callsound(sn,ii);
                    break;
                }
                j = nextspritestat[j];
            }
            return;


        case 28:
            //activate the rest of them

            j = headspritesect[sn];
            while(j >= 0)
            {
                if(sprite[j].statnum==3 && (sprite[j].lotag&0xff)==21)
                    break; //Found it
                j = nextspritesect[j];
            }

            j = sprite[j].hitag;

            l = headspritestat[3];
            while(l >= 0)
            {
                if( (sprite[l].lotag&0xff)==21 && !hittype[l].temp_data[0] &&
                    (sprite[l].hitag) == j )
                    hittype[l].temp_data[0] = 1;
                l = nextspritestat[l];
            }
            callsound(sn,ii);

            return;
    }
}



void operaterespawns(short low)
{
    short i, j, nexti;

    i = headspritestat[11];
    while(i >= 0)
    {
        nexti = nextspritestat[i];
        if(SLT == low) switch(PN)
        {
            case RESPAWN:
#if DUKE3D_GAMEVER < GAMEVER_13D
                // 1.1 shows the star only for monsters, and only with monsters on.
                if( ud.monsters_off == 0 && badguypic(SHT) )
                {
                    j = spawn(i,TRANSPORTERSTAR);
                    sprite[j].z -= (32<<8);
                }
#else
                if( badguypic(SHT) && ud.monsters_off ) break;

                j = spawn(i,TRANSPORTERSTAR);
                sprite[j].z -= (32<<8);
#endif

                sprite[i].extra = 66-12;   // Just a way to killit
                break;
        }
        i = nexti;
    }
}

void operateactivators(short low,short snum)
{
    short i, j, k, *p, nexti;
    walltype *wal;

    for(i=numcyclers-1;i>=0;i--)
    {
        p = &cyclers[i][0];

        if(p[4] == low)
        {
            p[5] = !p[5];

            sector[p[0]].floorshade = sector[p[0]].ceilingshade = p[3];
            wal = &wall[sector[p[0]].wallptr];
            for(j=sector[p[0]].wallnum;j > 0;j--,wal++)
                wal->shade = p[3];
        }
    }

    i = headspritestat[8];
    k = -1;
    while(i >= 0)
    {
        if(sprite[i].lotag == low)
        {
            if( sprite[i].picnum == ACTIVATORLOCKED )
            {
                if(sector[SECT].lotag&16384)
                    sector[SECT].lotag &= 65535-16384;
                else
                    sector[SECT].lotag |= 16384;

                if(snum >= 0)
                {
                    if(sector[SECT].lotag&16384)
                        FTA(4,&ps[snum]);
                    else FTA(8,&ps[snum]);
                }
            }
            else
            {
#if DUKE3D_GAMEVER >= GAMEVER_14
                switch(SHT)
                {
                    case 0:
                        break;
                    case 1:
                        if(sector[SECT].floorz != sector[SECT].ceilingz)
                        {
                            i = nextspritestat[i];
                            continue;
                        }
                        break;
                    case 2:
                        if(sector[SECT].floorz == sector[SECT].ceilingz)
                        {
                            i = nextspritestat[i];
                            continue;
                        }
                        break;
                }
#endif

                if( sector[sprite[i].sectnum].lotag < 3 )
                {
                    j = headspritesect[sprite[i].sectnum];
                    while(j >= 0)
                    {
                        if( sprite[j].statnum == 3 ) switch(sprite[j].lotag)
                        {
#if DUKE3D_GAMEVER < GAMEVER_13D
                            case 36: // 1.1 only switches SE36 on
                                hittype[j].temp_data[0] = 1;
                                break;
#else
                            case 36:
#endif
                            case 31:
                            case 32:
#if DUKE3D_GAMEVER >= GAMEVER_14
                            case 18:
#endif
                                hittype[j].temp_data[0] = 1-hittype[j].temp_data[0];
                                callsound(SECT,j);
                                break;
                        }
                        j = nextspritesect[j];
                    }
                }

                if( k == -1 && (sector[SECT].lotag&0xff) == 22 )
                    k = callsound(SECT,i);

                operatesectors(SECT,i);
            }
        }
        i = nextspritestat[i];
     }

    operaterespawns(low);
}

void operatemasterswitches(short low)
{
    short i;

    i = headspritestat[6];
    while(i >= 0)
    {
        if( PN == MASTERSWITCH && SLT == low && SP == 0 )
            SP = 1;
        i = nextspritestat[i];
    }
}



void operateforcefields(short s, short low)
{
    short i, p;

#if DUKE3D_GAMEVER < GAMEVER_13D
    for(p=numanimwalls-1;p>=0;p--)
#else
    for(p=numanimwalls;p>=0;p--)
#endif
    {
        i = animwall[p].wallnum;

        if(low == wall[i].lotag || low == -1)
            switch(wall[i].overpicnum)
        {
            case W_FORCEFIELD  :
            case W_FORCEFIELD+1:
            case W_FORCEFIELD+2:
            case BIGFORCE:

                animwall[p].tag = 0;

#if DUKE3D_GAMEVER < GAMEVER_13D
                // 1.1 ignores s (it takes only the lotag) and tints the sector
                // beyond the field: pal 0 off, 2 on. 1.1 writes the pals with no
                // nextsector test; a one-sided wall is skipped here.
                (void)s;
                wall[i].cstat = wall[i].cstat ? 0 : 85;
                if( wall[i].nextsector >= 0 )
                    sector[wall[i].nextsector].ceilingpal =
                        sector[wall[i].nextsector].floorpal = wall[i].cstat ? 2 : 0;
#else
                if( wall[i].cstat )
                {
                    wall[i].cstat   = 0;

                    if( s >= 0 && sprite[s].picnum == SECTOREFFECTOR &&
                        sprite[s].lotag == 30)
                            wall[i].lotag = 0;
                }
                else
                    wall[i].cstat = 85;
#endif
                break;
        }
    }
}


// 1.3D played the switch sounds at the viewed player's height.
#if DUKE3D_GAMEVER < GAMEVER_14
#define SWITCHSOUNDZ ps[screenpeek].posz
#else
#define SWITCHSOUNDZ ps[snum].posz
#endif

char checkhitswitch(short snum,int32_t w,char switchtype)
{
    char switchpal;
    short i, x, lotag,hitag,picnum,correctdips,numdips;
    int32_t sx,sy;

    if(w < 0) return 0;
    correctdips = 1;
    numdips = 0;

    if(switchtype == 1) // A wall sprite
    {
        lotag = sprite[w].lotag; if(lotag == 0) return 0;
        hitag = sprite[w].hitag;
        sx = sprite[w].x;
        sy = sprite[w].y;
        picnum = sprite[w].picnum;
        switchpal = sprite[w].pal;
    }
    else
    {
        lotag = wall[w].lotag; if(lotag == 0) return 0;
        hitag = wall[w].hitag;
        sx = wall[w].x;
        sy = wall[w].y;
        picnum = wall[w].picnum;
        switchpal = wall[w].pal;
    }

    switch(picnum)
    {
        case DIPSWITCH:
        case DIPSWITCH+1:
        case TECHSWITCH:
        case TECHSWITCH+1:
        case ALIENSWITCH:
        case ALIENSWITCH+1:
            break;
        case ACCESSSWITCH:
        case ACCESSSWITCH2:
            if(ps[snum].access_incs == 0)
            {
                if( switchpal == 0 )
                {
                    if( (ps[snum].got_access&1) )
                        ps[snum].access_incs = 1;
                    else FTA(70,&ps[snum]);
                }

                else if( switchpal == 21 )
                {
                    if( ps[snum].got_access&2 )
                        ps[snum].access_incs = 1;
                    else FTA(71,&ps[snum]);
                }

                else if( switchpal == 23 )
                {
                    if( ps[snum].got_access&4 )
                        ps[snum].access_incs = 1;
                    else FTA(72,&ps[snum]);
                }

                if( ps[snum].access_incs == 1 )
                {
                    if(switchtype == 0)
                        ps[snum].access_wallnum = w;
                    else
                        ps[snum].access_spritenum = w;
                }

                return 0;
            }
        case DIPSWITCH2:
        case DIPSWITCH2+1:
        case DIPSWITCH3:
        case DIPSWITCH3+1:
        case MULTISWITCH:
        case MULTISWITCH+1:
        case MULTISWITCH+2:
        case MULTISWITCH+3:
        case PULLSWITCH:
        case PULLSWITCH+1:
        case HANDSWITCH:
        case HANDSWITCH+1:
        case SLOTDOOR:
        case SLOTDOOR+1:
        case LIGHTSWITCH:
        case LIGHTSWITCH+1:
        case SPACELIGHTSWITCH:
        case SPACELIGHTSWITCH+1:
        case SPACEDOORSWITCH:
        case SPACEDOORSWITCH+1:
        case FRANKENSTINESWITCH:
        case FRANKENSTINESWITCH+1:
        case LIGHTSWITCH2:
        case LIGHTSWITCH2+1:
        case POWERSWITCH1:
        case POWERSWITCH1+1:
        case LOCKSWITCH1:
        case LOCKSWITCH1+1:
        case POWERSWITCH2:
        case POWERSWITCH2+1:
            if( check_activator_motion( lotag ) ) return 0;
            break;
        default:
            if( isadoorwall(picnum) == 0 ) return 0;
            break;
    }

    i = headspritestat[0];
    while(i >= 0)
    {
        if( lotag == SLT ) switch(PN)
        {
            case DIPSWITCH:
            case TECHSWITCH:
            case ALIENSWITCH:
                if( switchtype == 1 && w == i ) PN++;
                else if( SHT == 0 ) correctdips++;
                numdips++;
                break;
            case TECHSWITCH+1:
            case DIPSWITCH+1:
            case ALIENSWITCH+1:
                if( switchtype == 1 && w == i ) PN--;
                else if( SHT == 1 ) correctdips++;
                numdips++;
                break;
            case MULTISWITCH:
            case MULTISWITCH+1:
            case MULTISWITCH+2:
            case MULTISWITCH+3:
                sprite[i].picnum++;
                if( sprite[i].picnum > (MULTISWITCH+3) )
                    sprite[i].picnum = MULTISWITCH;
                break;
            case ACCESSSWITCH:
            case ACCESSSWITCH2:
            case SLOTDOOR:
            case LIGHTSWITCH:
            case SPACELIGHTSWITCH:
            case SPACEDOORSWITCH:
            case FRANKENSTINESWITCH:
            case LIGHTSWITCH2:
            case POWERSWITCH1:
            case LOCKSWITCH1:
            case POWERSWITCH2:
            case HANDSWITCH:
            case PULLSWITCH:
            case DIPSWITCH2:
            case DIPSWITCH3:
                sprite[i].picnum++;
                break;
            case PULLSWITCH+1:
            case HANDSWITCH+1:
            case LIGHTSWITCH2+1:
            case POWERSWITCH1+1:
            case LOCKSWITCH1+1:
            case POWERSWITCH2+1:
            case SLOTDOOR+1:
            case LIGHTSWITCH+1:
            case SPACELIGHTSWITCH+1:
            case SPACEDOORSWITCH+1:
            case FRANKENSTINESWITCH+1:
            case DIPSWITCH2+1:
            case DIPSWITCH3+1:
                sprite[i].picnum--;
                break;
        }
        i = nextspritestat[i];
    }

    for(i=0;i<numwalls;i++)
    {
        x = i;
        if(lotag == wall[x].lotag)
            switch(wall[x].picnum)
        {
            case DIPSWITCH:
            case TECHSWITCH:
            case ALIENSWITCH:
                if( switchtype == 0 && i == w ) wall[x].picnum++;
                else if( wall[x].hitag == 0 ) correctdips++;
                numdips++;
                break;
            case DIPSWITCH+1:
            case TECHSWITCH+1:
            case ALIENSWITCH+1:
                if( switchtype == 0 && i == w ) wall[x].picnum--;
                else if( wall[x].hitag == 1 ) correctdips++;
                numdips++;
                break;
            case MULTISWITCH:
            case MULTISWITCH+1:
            case MULTISWITCH+2:
            case MULTISWITCH+3:
                wall[x].picnum++;
                if(wall[x].picnum > (MULTISWITCH+3) )
                    wall[x].picnum = MULTISWITCH;
                break;
            case ACCESSSWITCH:
            case ACCESSSWITCH2:
            case SLOTDOOR:
            case LIGHTSWITCH:
            case SPACELIGHTSWITCH:
            case SPACEDOORSWITCH:
            case LIGHTSWITCH2:
            case POWERSWITCH1:
            case LOCKSWITCH1:
            case POWERSWITCH2:
            case PULLSWITCH:
            case HANDSWITCH:
            case DIPSWITCH2:
            case DIPSWITCH3:
                wall[x].picnum++;
                break;
            case HANDSWITCH+1:
            case PULLSWITCH+1:
            case LIGHTSWITCH2+1:
            case POWERSWITCH1+1:
            case LOCKSWITCH1+1:
            case POWERSWITCH2+1:
            case SLOTDOOR+1:
            case LIGHTSWITCH+1:
            case SPACELIGHTSWITCH+1:
            case SPACEDOORSWITCH+1:
            case DIPSWITCH2+1:
            case DIPSWITCH3+1:
                wall[x].picnum--;
                break;
        }
    }

    if(lotag == (short) 65535)
    {
        ps[myconnectindex].gm = MODE_EOL;
        if(ud.from_bonus)
        {
            ud.level_number = ud.from_bonus;
            ud.m_level_number = ud.level_number;
            ud.from_bonus = 0;
        }
        else
        {
            ud.level_number++;
#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 doesn't wrap; main ends the episode at level 5
            if( (ud.volume_number && ud.level_number > 10 ) || ( ud.volume_number == 0 && ud.level_number > 5 ) )
                ud.level_number = 0;
#endif
            ud.m_level_number = ud.level_number;
        }
        return 1;
    }

    switch(picnum)
    {
        default:
            if(isadoorwall(picnum) == 0) break;
        case DIPSWITCH:
        case DIPSWITCH+1:
        case TECHSWITCH:
        case TECHSWITCH+1:
        case ALIENSWITCH:
        case ALIENSWITCH+1:
            if( picnum == DIPSWITCH  || picnum == DIPSWITCH+1 ||
                picnum == ALIENSWITCH || picnum == ALIENSWITCH+1 ||
                picnum == TECHSWITCH || picnum == TECHSWITCH+1 )
            {
                if( picnum == ALIENSWITCH || picnum == ALIENSWITCH+1)
                {
                    if(switchtype == 1)
                        xyzsound(ALIEN_SWITCH1,w,sx,sy,SWITCHSOUNDZ);
                    else xyzsound(ALIEN_SWITCH1,ps[snum].i,sx,sy,SWITCHSOUNDZ);
                }
                else
                {
                    if(switchtype == 1)
                        xyzsound(SWITCH_ON,w,sx,sy,SWITCHSOUNDZ);
                    else xyzsound(SWITCH_ON,ps[snum].i,sx,sy,SWITCHSOUNDZ);
                }
                if(numdips != correctdips) break;
                xyzsound(END_OF_LEVEL_WARN,ps[snum].i,sx,sy,ps[snum].posz);
            }
        case DIPSWITCH2:
        case DIPSWITCH2+1:
        case DIPSWITCH3:
        case DIPSWITCH3+1:
        case MULTISWITCH:
        case MULTISWITCH+1:
        case MULTISWITCH+2:
        case MULTISWITCH+3:
        case ACCESSSWITCH:
        case ACCESSSWITCH2:
        case SLOTDOOR:
        case SLOTDOOR+1:
        case LIGHTSWITCH:
        case LIGHTSWITCH+1:
        case SPACELIGHTSWITCH:
        case SPACELIGHTSWITCH+1:
        case SPACEDOORSWITCH:
        case SPACEDOORSWITCH+1:
        case FRANKENSTINESWITCH:
        case FRANKENSTINESWITCH+1:
        case LIGHTSWITCH2:
        case LIGHTSWITCH2+1:
        case POWERSWITCH1:
        case POWERSWITCH1+1:
        case LOCKSWITCH1:
        case LOCKSWITCH1+1:
        case POWERSWITCH2:
        case POWERSWITCH2+1:
        case HANDSWITCH:
        case HANDSWITCH+1:
        case PULLSWITCH:
        case PULLSWITCH+1:

                if( picnum == MULTISWITCH || picnum == (MULTISWITCH+1) ||
                    picnum == (MULTISWITCH+2) || picnum == (MULTISWITCH+3) )
                        lotag += picnum-MULTISWITCH;

                x = headspritestat[3];
                while(x >= 0)
                {
                   if( ((sprite[x].hitag) == lotag) )
                   {
                       switch(sprite[x].lotag)
                       {
                           case 12:
                                sector[sprite[x].sectnum].floorpal = 0;
                                hittype[x].temp_data[0]++;
                                if(hittype[x].temp_data[0] == 2)
                                   hittype[x].temp_data[0]++;

                                break;
                           case 24:
                           case 34:
                           case 25:
                               hittype[x].temp_data[4] = !hittype[x].temp_data[4];
#if DUKE3D_GAMEVER < GAMEVER_13D // 1.1 has the two quotes the other way round
                               if(hittype[x].temp_data[4])
                                   FTA(2,&ps[snum]);
                               else FTA(15,&ps[snum]);
#else
                               if(hittype[x].temp_data[4])
                                   FTA(15,&ps[snum]);
                               else FTA(2,&ps[snum]);
#endif
                               break;
                           case 21:
                               FTA(2,&ps[screenpeek]);
                               break;
                       }
                   }
                   x = nextspritestat[x];
                }

                operateactivators(lotag,snum);
                operateforcefields(ps[snum].i,lotag);
                operatemasterswitches(lotag);

                if( picnum == DIPSWITCH || picnum == DIPSWITCH+1 ||
                    picnum == ALIENSWITCH || picnum == ALIENSWITCH+1 ||
                    picnum == TECHSWITCH || picnum == TECHSWITCH+1 ) return 1;

                if( hitag == 0 && isadoorwall(picnum) == 0 )
                {
                    if(switchtype == 1)
                        xyzsound(SWITCH_ON,w,sx,sy,SWITCHSOUNDZ);
                    else xyzsound(SWITCH_ON,ps[snum].i,sx,sy,SWITCHSOUNDZ);
                }
                else if(hitag != 0)
                {
#if DUKE3D_GAMEVER < GAMEVER_13D // 1.1 plays both at the viewed player's height
                    if(switchtype == 1 && (soundm[hitag]&4) == 0)
                        xyzsound(hitag,w,sx,sy,SWITCHSOUNDZ);
                    else xyzsound(hitag,ps[snum].i,sx,sy,SWITCHSOUNDZ);
#else
                    if(switchtype == 1 && (soundm[hitag]&4) == 0)
                        xyzsound(hitag,w,sx,sy,ps[snum].posz);
                    else spritesound(hitag,ps[snum].i);
#endif
                }

               return 1;
    }
    return 0;
}


void activatebysector(short sect,short j)
{
    short i,didit;

    didit = 0;

    i = headspritesect[sect];
    while(i >= 0)
    {
        if(PN == ACTIVATOR)
        {
            operateactivators(SLT,-1);
            didit = 1;
#if DUKE3D_GAMEVER < GAMEVER_14
            return;
#endif
//            return;
        }
        i = nextspritesect[i];
    }

    if(didit == 0)
        operatesectors(sect,j);
}

void breakwall(short newpn,short spr,short dawallnum)
{
    wall[dawallnum].picnum = newpn;
    spritesound(VENT_BUST,spr);
    spritesound(GLASS_HEAVYBREAK,spr);
    lotsofglass(spr,dawallnum,10);
}

#if DUKE3D_GAMEVER < GAMEVER_14
// 1.3D's broken techwalls play a lighter glass sound, apart from the first.
static void breaktechwall13(short newpn,short spr,short dawallnum,short glasssound)
{
    wall[dawallnum].picnum = newpn;
    spritesound(VENT_BUST,spr);
    spritesound(glasssound,spr);
    lotsofglass(spr,dawallnum,10);
}
#endif

void checkhitwall(short spr,short dawallnum,int32_t x,int32_t y,int32_t z,short atwith)
{
    short j, i, sn = -1, darkestwall;
    signed char nfloors,nceilings;
    short nextj;
    walltype *wal;
    spritetype *s;

    wal = &wall[dawallnum];

    if(wal->overpicnum == MIRROR)
    {
        switch(atwith)
        {
            case HEAVYHBOMB:
            case RADIUSEXPLOSION:
            case RPG:
            case HYDRENT:
            case SEENINE:
#if DUKE3D_GAMEVER >= GAMEVER_13D
            case OOZFILTER:
#endif
            case EXPLODINGBARREL:
                lotsofglass(spr,dawallnum,70);
                wal->cstat &= ~16;
                wal->overpicnum = MIRRORBROKE;
                spritesound(GLASS_HEAVYBREAK,spr);
                return;
        }
    }

   if( ( (wal->cstat&16) || wal->overpicnum == BIGFORCE ) && wal->nextsector >= 0 )
#if DUKE3D_GAMEVER < GAMEVER_13D
       if( wal->overpicnum >= 0 )
#endif
       if( sector[wal->nextsector].floorz > z )
           if( sector[wal->nextsector].floorz-sector[wal->nextsector].ceilingz )
               switch(wal->overpicnum)
    {
        case W_FORCEFIELD:
        case W_FORCEFIELD+1:
        case W_FORCEFIELD+2:
            wal->extra = 1; // tell the forces to animate
        case BIGFORCE:
            updatesector(x,y,&sn);
            if( sn < 0 ) return;

            if(atwith == -1)
                i = EGS(sn,x,y,z,FORCERIPPLE,-127,8,8,0,0,0,spr,5);
            else
            {
                if(atwith == CHAINGUN)
                    i = EGS(sn,x,y,z,FORCERIPPLE,-127,16+sprite[spr].xrepeat,16+sprite[spr].yrepeat,0,0,0,spr,5);
                else i = EGS(sn,x,y,z,FORCERIPPLE,-127,32,32,0,0,0,spr,5);
            }

            CS |= 18+128;
            SA = getangle(wal->x-wall[wal->point2].x,
                wal->y-wall[wal->point2].y)-512;

            spritesound(SOMETHINGHITFORCE,i);

            return;

        case FANSPRITE:
#if DUKE3D_GAMEVER < GAMEVER_14
            // 1.3D breaks the wall's own picnum and throws scrap instead of
            // playing sounds. Its scrap owner was whatever a register held;
            // the hitting sprite stands in for it.
            wal->picnum = FANSPRITEBROKE;
            wal->cstat &= 65535-65;
            for(j=0;j<16;j++)
            {
                int32_t zv = -512-(TRAND&2047), vel = (TRAND&63)+64;
                int32_t ang = TRAND&2047, pn = SCRAP6+(TRAND&15);

                EGS(sprite[spr].sectnum,sprite[spr].x,sprite[spr].y,sprite[spr].z-(12<<8),
                    pn,-8,48,48,ang,vel,zv,spr,5);
            }
#else
            wal->overpicnum = FANSPRITEBROKE;
            wal->cstat &= 65535-65;
            if(wal->nextwall >= 0)
            {
                wall[wal->nextwall].overpicnum = FANSPRITEBROKE;
                wall[wal->nextwall].cstat &= 65535-65;
            }
            spritesound(VENT_BUST,spr);
            spritesound(GLASS_BREAKING,spr);
#endif
            return;

        case GLASS:
            updatesector(x,y,&sn); if( sn < 0 ) return;
            wal->overpicnum=GLASS2;
            lotsofglass(spr,dawallnum,10);
            wal->cstat = 0;

            if(wal->nextwall >= 0)
                wall[wal->nextwall].cstat = 0;

            i = EGS(sn,x,y,z,SECTOREFFECTOR,0,0,0,ps[0].ang,0,0,spr,3);
            SLT = 128; T2 = 5; T3 = dawallnum;
            spritesound(GLASS_BREAKING,i);
            return;
        case STAINGLASS1:
            updatesector(x,y,&sn); if( sn < 0 ) return;
            lotsofcolourglass(spr,dawallnum,80);
            wal->cstat = 0;
            if(wal->nextwall >= 0)
                wall[wal->nextwall].cstat = 0;
            spritesound(VENT_BUST,spr);
            spritesound(GLASS_BREAKING,spr);
            return;
    }

    switch(wal->picnum)
    {
            case COLAMACHINE:
            case VENDMACHINE:
#if DUKE3D_GAMEVER < GAMEVER_14
                wal->picnum += 2;
                spritesound(VENT_BUST,spr);
                spritesound(GLASS_BREAKING,spr);
#else
                breakwall(wal->picnum+2,spr,dawallnum);
                spritesound(VENT_BUST,spr);
#endif
                return;

            case OJ:
            case FEMPIC2:
            case FEMPIC3:

            case SCREENBREAK6:
            case SCREENBREAK7:
            case SCREENBREAK8:

            case SCREENBREAK1:
            case SCREENBREAK2:
            case SCREENBREAK3:
            case SCREENBREAK4:
            case SCREENBREAK5:

            case SCREENBREAK9:
            case SCREENBREAK10:
            case SCREENBREAK11:
            case SCREENBREAK12:
            case SCREENBREAK13:
#if DUKE3D_GAMEVER >= GAMEVER_14
            case SCREENBREAK14:
            case SCREENBREAK15:
            case SCREENBREAK16:
            case SCREENBREAK17:
            case SCREENBREAK18:
            case SCREENBREAK19:
#endif
#if DUKE3D_GAMEVER >= GAMEVER_13D
            case BORNTOBEWILDSCREEN:
#endif

                lotsofglass(spr,dawallnum,30);
                wal->picnum=W_SCREENBREAK+(TRAND%3);
                spritesound(GLASS_HEAVYBREAK,spr);
                return;

#if DUKE3D_GAMEVER < GAMEVER_14
            case W_TECHWALL1:
                breaktechwall13(W_HITTECHWALL1,spr,dawallnum,GLASS_HEAVYBREAK);
                return;
            case W_TECHWALL2:
                breaktechwall13(W_HITTECHWALL2,spr,dawallnum,GLASS_BREAKING);
                return;
            case W_TECHWALL3:
                breaktechwall13(W_HITTECHWALL3,spr,dawallnum,GLASS_BREAKING);
                return;
            case W_TECHWALL4:
                breaktechwall13(W_HITTECHWALL4,spr,dawallnum,GLASS_BREAKING);
                return;
#else
            case W_TECHWALL5:
            case W_TECHWALL6:
            case W_TECHWALL7:
            case W_TECHWALL8:
            case W_TECHWALL9:
                breakwall(wal->picnum+1,spr,dawallnum);
                return;
            case W_MILKSHELF:
                breakwall(W_MILKSHELFBROKE,spr,dawallnum);
                return;

            case W_TECHWALL10:
                breakwall(W_HITTECHWALL10,spr,dawallnum);
                return;

            case W_TECHWALL1:
            case W_TECHWALL11:
            case W_TECHWALL12:
            case W_TECHWALL13:
            case W_TECHWALL14:
                breakwall(W_HITTECHWALL1,spr,dawallnum);
                return;

            case W_TECHWALL15:
                breakwall(W_HITTECHWALL15,spr,dawallnum);
                return;

            case W_TECHWALL16:
                breakwall(W_HITTECHWALL16,spr,dawallnum);
                return;

            case W_TECHWALL2:
                breakwall(W_HITTECHWALL2,spr,dawallnum);
                return;

            case W_TECHWALL3:
                breakwall(W_HITTECHWALL3,spr,dawallnum);
                return;

            case W_TECHWALL4:
                breakwall(W_HITTECHWALL4,spr,dawallnum);
                return;
#endif

            case ATM:
                wal->picnum = ATMBROKE;
                lotsofmoney(&sprite[spr],1+(TRAND&7));
                spritesound(GLASS_HEAVYBREAK,spr);
                break;

            case WALLLIGHT1:
            case WALLLIGHT2:
            case WALLLIGHT3:
            case WALLLIGHT4:
            case TECHLIGHT2:
            case TECHLIGHT4:

#if DUKE3D_GAMEVER < GAMEVER_14
                spritesound(GLASS_HEAVYBREAK,spr);
#else
                if( rnd(128) )
                    spritesound(GLASS_HEAVYBREAK,spr);
                else spritesound(GLASS_BREAKING,spr);
#endif
                lotsofglass(spr,dawallnum,30);

                if(wal->picnum == WALLLIGHT1)
                    wal->picnum = WALLLIGHTBUST1;

                if(wal->picnum == WALLLIGHT2)
                    wal->picnum = WALLLIGHTBUST2;

                if(wal->picnum == WALLLIGHT3)
                    wal->picnum = WALLLIGHTBUST3;

                if(wal->picnum == WALLLIGHT4)
                    wal->picnum = WALLLIGHTBUST4;

                if(wal->picnum == TECHLIGHT2)
                    wal->picnum = TECHLIGHTBUST2;

                if(wal->picnum == TECHLIGHT4)
                    wal->picnum = TECHLIGHTBUST4;

                if(!wal->lotag) return;

                sn = wal->nextsector;
                if(sn < 0) return;
                darkestwall = 0;

                wal = &wall[sector[sn].wallptr];
                for(i=sector[sn].wallnum;i > 0;i--,wal++)
                    if(wal->shade > darkestwall)
                        darkestwall=wal->shade;

                j = TRAND&1;
                i= headspritestat[3];
                while(i >= 0)
                {
                    if(SHT == wall[dawallnum].lotag && SLT == 3 )
                    {
                        T3 = j;
                        T4 = darkestwall;
                        T5 = 1;
                    }
                    i = nextspritestat[i];
                }
                break;
    }
}


void checkplayerhurt(struct player_struct *p,short j)
{
    if( (j&49152) == 49152 )
    {
        j &= (MAXSPRITES-1);

        switch(sprite[j].picnum)
        {
            case CACTUS:
                if(p->hurt_delay < 8 )
                {
                    sprite[p->i].extra -= 5;

                    p->hurt_delay = 16;
                    p->pals_time = 32;
                    p->pals[0] = 32;
                    p->pals[1] = 0;
                    p->pals[2] = 0;
                    spritesound(DUKE_LONGTERM_PAIN,p->i);
                }
                break;
        }
        return;
    }

    if( (j&49152) != 32768) return;
    j &= (MAXWALLS-1);

    if( p->hurt_delay > 0 ) p->hurt_delay--;
#if DUKE3D_GAMEVER < GAMEVER_13D // 1.1 hurts with the field off too
    else switch(wall[j].overpicnum)
#else
    else if( wall[j].cstat&85 ) switch(wall[j].overpicnum)
#endif
    {
        case W_FORCEFIELD:
        case W_FORCEFIELD+1:
        case W_FORCEFIELD+2:
             sprite[p->i].extra -= 5;

             p->hurt_delay = 16;
             p->pals_time = 32;
             p->pals[0] = 32;
             p->pals[1] = 0;
             p->pals[2] = 0;

             p->posxv = -(sintable[(p->ang+512)&2047]<<8);
             p->posyv = -(sintable[(p->ang)&2047]<<8);
             spritesound(DUKE_LONGTERM_PAIN,p->i);

             checkhitwall(p->i,j,
                 p->posx+(sintable[(p->ang+512)&2047]>>9),
                 p->posy+(sintable[p->ang&2047]>>9),
                 p->posz,-1);

            break;

        case BIGFORCE:
            p->hurt_delay = 26;
            checkhitwall(p->i,j,
                p->posx+(sintable[(p->ang+512)&2047]>>9),
                p->posy+(sintable[p->ang&2047]>>9),
                p->posz,-1);
            break;

    }
}


char checkhitceiling(short sn)
{
    short i, j, q, darkestwall, darkestceiling;
    signed char nfloors,nceilings;
    walltype *wal;

    switch(sector[sn].ceilingpicnum)
    {
        case WALLLIGHT1:
        case WALLLIGHT2:
        case WALLLIGHT3:
        case WALLLIGHT4:
        case TECHLIGHT2:
        case TECHLIGHT4:

                ceilingglass(ps[myconnectindex].i,sn,10);
                spritesound(GLASS_BREAKING,ps[screenpeek].i);

                if(sector[sn].ceilingpicnum == WALLLIGHT1)
                    sector[sn].ceilingpicnum = WALLLIGHTBUST1;

                if(sector[sn].ceilingpicnum == WALLLIGHT2)
                    sector[sn].ceilingpicnum = WALLLIGHTBUST2;

                if(sector[sn].ceilingpicnum == WALLLIGHT3)
                    sector[sn].ceilingpicnum = WALLLIGHTBUST3;

                if(sector[sn].ceilingpicnum == WALLLIGHT4)
                    sector[sn].ceilingpicnum = WALLLIGHTBUST4;

                if(sector[sn].ceilingpicnum == TECHLIGHT2)
                    sector[sn].ceilingpicnum = TECHLIGHTBUST2;

                if(sector[sn].ceilingpicnum == TECHLIGHT4)
                    sector[sn].ceilingpicnum = TECHLIGHTBUST4;


                if(!sector[sn].hitag)
                {
                    i = headspritesect[sn];
                    while(i >= 0)
                    {
                        if( PN == SECTOREFFECTOR && SLT == 12 )
                        {
                            j = headspritestat[3];
                            while(j >= 0)
                            {
                                if( sprite[j].hitag == SHT )
                                    hittype[j].temp_data[3] = 1;
                                j = nextspritestat[j];
                            }
                            break;
                        }
                        i = nextspritesect[i];
                    }
                }

                i = headspritestat[3];
                j = TRAND&1;
                while(i >= 0)
                {
                    if(SHT == (sector[sn].hitag) && SLT == 3 )
                    {
                        T3 = j;
                        T5 = 1;
                    }
                    i = nextspritestat[i];
                }

                return 1;
    }

    return 0;
}                                     

#if DUKE3D_GAMEVER < GAMEVER_13D
// 1.1's checkhitsprite (G11-S10..S12). Its case set, the stripper damage case,
// the THROWFLAME reactions and the default knockback differ throughout, so it
// is a separate copy.
void checkhitsprite(short i,short sn)
{
    short j, k, p;
    spritetype *s;

    i &= (MAXSPRITES-1);

    switch(PN)
    {
        case OCEANSPRITE1:
        case OCEANSPRITE2:
        case OCEANSPRITE3:
        case OCEANSPRITE4:
        case OCEANSPRITE5:
            spawn(i,SMALLSMOKE);
            deletesprite(i);
            break;
        case QUEBALL:
        case STRIPEBALL:
            if(sprite[sn].picnum == QUEBALL || sprite[sn].picnum == STRIPEBALL)
            {
                sprite[sn].xvel = (sprite[i].xvel>>1)+(sprite[i].xvel>>2);
                sprite[sn].ang -= (SA<<1)+1024;
                SA = getangle(SX-sprite[sn].x,SY-sprite[sn].y)-512;
                if(Sound[POOLBALLHIT].num < 2)
                    spritesound(POOLBALLHIT,i);
            }
            else
            {
                if( TRAND&3 )
                {
                    sprite[i].xvel = 164;
                    sprite[i].ang = sprite[sn].ang;
                }
                else
                {
                    lotsofglass(i,-1,3);
                    deletesprite(i);
                }
            }
            break;
        case TREE1:
        case TREE2:
        case TIRE:
        case CONE:
        case BOX:
            switch(sprite[sn].picnum)
            {
                case RADIUSEXPLOSION:
                case RPG:
                case FIRELASER:
                case HYDRENT:
                case HEAVYHBOMB:
                case THROWFLAME:
                    if(T1 == 0)
                    {
                        CS &= ~257;
                        T1 = 1;
                        spawn(i,BURNING);
                    }
                    break;
            }
            break;
        case CACTUS:
            switch(sprite[sn].picnum)
            {
                case RADIUSEXPLOSION:
                case RPG:
                case FIRELASER:
                case HYDRENT:
                case HEAVYHBOMB:
                case THROWFLAME:
                    for(k=0;k<64;k++)
                    {
                        {
                            // Fix: draw the random numbers in the DOS executable's (Watcom's) argument order.
                            int32_t r_zv = -(TRAND&4095)-(sprite[i].zvel>>2);
                            int32_t r_vel = (TRAND&63)+64;
                            int32_t r_ang = TRAND&2047;
                            int32_t r_pn = SCRAP3+(TRAND&3);
                            int32_t r_z = SZ-(TRAND%(48<<8));
                            j = EGS(SECT,SX,SY,r_z,r_pn,-8,48,48,r_ang,r_vel,r_zv,i,5);
                        }
                        sprite[j].pal = 8;
                    }

                    if(PN == CACTUS)
                        PN = CACTUSBROKE;
                    CS &= ~257;
                    break;
            }
            break;

        case HANGLIGHT:
            for(k=0;k<6;k++)
                {
                    // Fix: draw the random numbers in the DOS executable's (Watcom's) argument order.
                    int32_t r_zv = -(TRAND&4095)-(sprite[i].zvel>>2);
                    int32_t r_vel = (TRAND&63)+64;
                    int32_t r_ang = TRAND&2047;
                    int32_t r_pn = SCRAP1+(TRAND&15);
                    EGS(SECT,SX,SY,SZ-(8<<8),r_pn,-8,48,48,r_ang,r_vel,r_zv,i,5);
                }
            spritesound(GLASS_HEAVYBREAK,i);
            deletesprite(i);
            break;

        case FANSPRITE:
            PN = FANSPRITEBROKE;
            CS &= (65535-257);
            if( sector[SECT].floorpicnum == FANSHADOW )
                sector[SECT].floorpicnum = FANSHADOWBROKE;

            s = &sprite[i];
            for(j=0;j<16;j++) RANDOMSCRAP;

            break;
        case WATERFOUNTAIN:
        case WATERFOUNTAIN+1:
        case WATERFOUNTAIN+2:
        case WATERFOUNTAIN+3:
            PN = WATERFOUNTAINBROKE;
            spawn(i,TOILETWATER);
            break;
        case SATELITE:
        case FUELPOD:
        case SOLARPANNEL:
        case ANTENNA:
            if(sprite[sn].extra != *SCRIPT_PTR(actorscrptr[SHOTSPARK1]) )
            {
                for(j=0;j<15;j++)
                    {
                        // Fix: draw the random numbers in the DOS executable's (Watcom's) argument order.
                        int32_t r_zv = -(TRAND&511)-256;
                        int32_t r_vel = (TRAND&127)+64;
                        int32_t r_ang = TRAND&2047;
                        int32_t r_pn = SCRAP1+(TRAND&15);
                        EGS(SECT,SX,SY,sector[SECT].floorz-(12<<8)-(j<<9),r_pn,-8,64,64,r_ang,r_vel,r_zv,i,5);
                    }
                spawn(i,EXPLOSION2);
                deletesprite(i);
            }
            break;
        case BOTTLE1:
        case BOTTLE2:
        case BOTTLE3:
        case BOTTLE4:
        case BOTTLE5:
        case BOTTLE6:
        case BOTTLE7:
        case BOTTLE8:
        case BOTTLE10:
        case BOTTLE11:
        case BOTTLE12:
        case BOTTLE13:
        case BOTTLE14:
        case BOTTLE15:
        case BOTTLE16:
        case BOTTLE17:
        case BOTTLE18:
        case BOTTLE19:
        case WATERFOUNTAINBROKE:
        case DOMELITE:
            if(PN == BOTTLE10)
                lotsofmoney(&sprite[i],4+(TRAND&3));

            spritesound(GLASS_BREAKING,i);
            SA = TRAND&2047;
            lotsofglass(i,-1,8);
            deletesprite(i);
            break;
        case HYDROPLANT:
            PN = BROKEHYDROPLANT;
            spritesound(GLASS_BREAKING,i);
            lotsofglass(i,-1,10);
            break;

        case FORCESPHERE:
            sprite[i].xrepeat = 0;
            hittype[OW].temp_data[0] = 32;
            hittype[OW].temp_data[1] = !hittype[OW].temp_data[1];
            hittype[OW].temp_data[2] ++;
            spawn(i,EXPLOSION2);
            break;

        case BROKEHYDROPLANT:
            if(CS&1)
            {
                spritesound(GLASS_BREAKING,i);
                SZ += 16<<8;
                CS = 0;
                lotsofglass(i,-1,5);
            }
            break;

        case TOILET:
            PN = TOILETBROKE;
            CS |= (TRAND&1)<<2;
            CS &= ~257;
            spawn(i,TOILETWATER);
            spritesound(GLASS_BREAKING,i);
            break;

        case STALL:
            PN = STALLBROKE;
            CS |= (TRAND&1)<<2;
            CS &= ~257;
            spawn(i,TOILETWATER);
            spritesound(GLASS_HEAVYBREAK,i);
            break;

        case HYDRENT:
            PN = BROKEFIREHYDRENT;
            spawn(i,TOILETWATER);
            spritesound(GLASS_HEAVYBREAK,i);
            break;

        case GRATE1:
            PN = BGRATE1;
            CS &= (65535-256-1);
            spritesound(VENT_BUST,i);
            break;

        case CIRCLEPANNEL:
            PN = CIRCLEPANNELBROKE;
            CS &= (65535-256-1);
            spritesound(VENT_BUST,i);
            break;
        case PANNEL1:
        case PANNEL2:
            PN = BPANNEL1;
            CS &= (65535-256-1);
            spritesound(VENT_BUST,i);
            break;

        // Every pipe breaks to the first broken tile of its set, with two steam sprites.
        case PIPE1:
        case PIPE2:
        case PIPE3:
        case PIPE4:
        case PIPE5:
        case PIPE6:
            if(PN == PIPE4 || PN == PIPE5 || PN == PIPE6)
                PN = PIPE4B;
            else PN = PIPE1B;

            j = spawn(i,STEAM);
            sprite[j].z = sector[SECT].floorz-(32<<8);
            j = spawn(i,STEAM);
            sprite[j].z = sector[SECT].floorz-(32<<8);
            break;

        case MONK:
            spritesound(SLT,i);
            spawn(i,SHT);
        case SPACEMARINE:
            sprite[i].extra -= sprite[sn].extra;
            if(sprite[i].extra > 0) break;
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT1);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT2);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT3);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT4);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT1);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT2);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT3);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT4);
            guts(&sprite[i],JIBS1,1,-1);
            guts(&sprite[i],JIBS2,2,-1);
            guts(&sprite[i],JIBS3,3,-1);
            guts(&sprite[i],JIBS4,4,-1);
            guts(&sprite[i],JIBS5,1,-1);
            guts(&sprite[i],JIBS3,6,-1);
            deletesprite(i);
            break;

        // The strippers take damage, scream and die here (G11-S10).
        case NAKED1:
        case PODFEM1:
        case FEM1:
        case FEM2:
        case FEM3:
        case FEM4:
            sprite[i].extra -= sprite[sn].extra;
            if(ud.lockout == 0)
                spritesound(LADY_SCREAM,i);

            if(sprite[i].extra >= 0)
            {
                j = spawn(sn,JIBS6);
                sprite[j].z += (4<<8);
                sprite[j].xvel = 16;
                sprite[j].xrepeat = sprite[j].yrepeat = 24;
                sprite[j].ang += 32-(TRAND&63);
                break;
            }

            // 1.1 reads the owner's sprite without a test; owners are never negative here.
            if(hittype[sn].owner >= 0 && sprite[hittype[sn].owner].picnum == APLAYER)
            {
                if(TRAND&1)
                    spritesound(DUKE_HIT_STRIPPER1,hittype[sn].owner);
                else spritesound(DUKE_HIT_STRIPPER2,hittype[sn].owner);
            }

            guts(&sprite[i],JIBS3,7,-1);
            guts(&sprite[i],JIBS5,1,-1);
            guts(&sprite[i],JIBS6,6,-1);
            shoot(i,BLOODSPLAT1);
            shoot(i,BLOODSPLAT2);
            shoot(i,BLOODSPLAT3);

            if(PN != NAKED1 && PN != FEM4 && PN != PODFEM1)
                lotsofmoney(&sprite[i],(TRAND&3)+3);

            spawn(i,BLOODPOOL);
            operaterespawns(SHT);

            if(PN == FEM2)
            {
                PN = BARBROKE; // 1.1's dead FEM2 tile
                CS = 0;
                break;
            }

            if(PN == PODFEM1)
                for(k=0;k<64;k++)
                {
                    {
                        // Fix: draw the random numbers in the DOS executable's (Watcom's) argument order.
                        int32_t r_zv = -(TRAND&4095)-(sprite[i].zvel>>2);
                        int32_t r_vel = (TRAND&63)+64;
                        int32_t r_ang = TRAND&2047;
                        int32_t r_pn = SCRAP3+(TRAND&3);
                        int32_t r_z = SZ-(TRAND%(48<<8));
                        j = EGS(SECT,SX,SY,r_z,r_pn,-8,48,48,r_ang,r_vel,r_zv,i,5);
                    }
                    sprite[j].pal = 8;
                }

            deletesprite(i);
            break;

        case CHAIR1:
        case CHAIR2:
            PN = BROKENCHAIR;
            CS = 0;
            break;
        case CHAIR3:
            deletesprite(i);
            break;
        case CLOCK:
            PN = BROKENCLOCK;
            spritesound(GLASS_BREAKING,i);
            break;
        case PLAYERONWATER:
            i = OW;
        default:
            if( sprite[sn].owner != i && sprite[i].statnum != 4)
            {
                if( badguy(&sprite[i]) == 1)
                {
                    if(sprite[sn].picnum == RPG) sprite[sn].extra <<= 1;

                    if( (PN < GREENSLIME || PN > GREENSLIME+7) && PN != ROTATEGUN )
                    {
                        j = spawn(sn,JIBS6);
                        sprite[j].z += (4<<8);
                        sprite[j].xvel = 16;
                        sprite[j].xrepeat = sprite[j].yrepeat = 24;
                        sprite[j].ang += 32-(TRAND&63);
                    }

                    j = sprite[sn].owner;

                    if( j >= 0 && sprite[j].picnum == APLAYER && PN != ROTATEGUN )
                        if( ps[sprite[j].yvel].curr_weapon == SHOTGUN_WEAPON )
                    {
                        shoot(i,BLOODSPLAT3);
                        shoot(i,BLOODSPLAT1);
                        shoot(i,BLOODSPLAT2);
                        shoot(i,BLOODSPLAT4);
                    }

                    if( PN != BOSS1 && PN != BOSS2 && PN != RECON && PN != ROTATEGUN )
                    {
                        SA = (sprite[sn].ang+1024)&2047;
                        if(sprite[i].xvel > 0) sprite[i].xvel = 0;
                        sprite[i].xvel -= (sprite[sn].extra<<2);
                    }

                    if(sprite[i].statnum == 2)
                    {
                        changespritestat(i,1);
                        hittype[i].timetosleep = 0;
                    }
                    if( RX < 40 && sprite[sn].picnum == SHRINKSPARK) return;

                    if( sprite[sn].picnum == THROWFLAME )
                    {
                        j = spawn(i,FRAMEEFFECT1);
                        sprite[j].xrepeat += 10;
                        sprite[j].yrepeat += 10;
                    }
                }

                if( sprite[i].statnum != 2 )
                {
                    hittype[i].picnum = sprite[sn].picnum;
                    hittype[i].extra += sprite[sn].extra;
                    hittype[i].ang = sprite[sn].ang;
                    hittype[i].owner = sprite[sn].owner;
                }

                if(sprite[i].statnum == 10)
                {
                    p = sprite[i].yvel;
                    if(ps[p].newowner >= 0)
                    {
                        ps[p].newowner = -1;
                        ps[p].posx = ps[p].oposx;
                        ps[p].posy = ps[p].oposy;
                        ps[p].posz = ps[p].oposz;
                        ps[p].ang = ps[p].oang;

                        // 1.1 doesn't updatesector; setpal stands in for its camera render flag.
                        setpal(&ps[p]);

                        j = headspritestat[1];
                        while(j >= 0)
                        {
                            if(sprite[j].picnum==CAMERA1) sprite[j].yvel = 0;
                            j = nextspritestat[j];
                        }
                    }

                    if( RX > 47 || sprite[sn].picnum != SHRINKSPARK )
                        if( sprite[hittype[i].owner].picnum != APLAYER)
                            if(ud.player_skill >= 3)
                                sprite[sn].extra += (sprite[sn].extra>>1);
                }

            }
            break;
    }
}
#else
void checkhitsprite(short i,short sn)
{
    short j, k, l, nextj, p;
    spritetype *s;

    i &= (MAXSPRITES-1);

    switch(PN)
    {
        case OCEANSPRITE1:
        case OCEANSPRITE2:
        case OCEANSPRITE3:
        case OCEANSPRITE4:
        case OCEANSPRITE5:
            spawn(i,SMALLSMOKE);
            deletesprite(i);
            break;
        case QUEBALL:
        case STRIPEBALL:
            if(sprite[sn].picnum == QUEBALL || sprite[sn].picnum == STRIPEBALL)
            {
                sprite[sn].xvel = (sprite[i].xvel>>1)+(sprite[i].xvel>>2);
                sprite[sn].ang -= (SA<<1)+1024;
                SA = getangle(SX-sprite[sn].x,SY-sprite[sn].y)-512;
                if(Sound[POOLBALLHIT].num < 2)
                    spritesound(POOLBALLHIT,i);
            }
            else
            {
                if( TRAND&3 )
                {
                    sprite[i].xvel = 164;
                    sprite[i].ang = sprite[sn].ang;
                }
                else
                {
                    lotsofglass(i,-1,3);
                    deletesprite(i);
                }
            }
            break;
        case TREE1:
        case TREE2:
        case TIRE:
        case CONE:
        case BOX:
            switch(sprite[sn].picnum)
            {
                case RADIUSEXPLOSION:
                case RPG:
                case FIRELASER:
                case HYDRENT:
                case HEAVYHBOMB:
                    if(T1 == 0)
                    {
                        CS &= ~257;
                        T1 = 1;
                        spawn(i,BURNING);
                    }
                    break;
            }
            break;
        case CACTUS:
//        case CACTUSBROKE:
            switch(sprite[sn].picnum)
            {
                case RADIUSEXPLOSION:
                case RPG:
                case FIRELASER:
                case HYDRENT:
                case HEAVYHBOMB:
                    for(k=0;k<64;k++)
                    {
                        {
                            // Fix: draw the random numbers in the DOS executable's (Watcom's) argument order.
                            int32_t r_zv = -(TRAND&4095)-(sprite[i].zvel>>2);
                            int32_t r_vel = (TRAND&63)+64;
                            int32_t r_ang = TRAND&2047;
                            int32_t r_pn = SCRAP3+(TRAND&3);
                            int32_t r_z = SZ-(TRAND%(48<<8));
                            j = EGS(SECT,SX,SY,r_z,r_pn,-8,48,48,r_ang,r_vel,r_zv,i,5);
                        }
                        sprite[j].pal = 8;
                    }

                    if(PN == CACTUS)
                        PN = CACTUSBROKE;
                    CS &= ~257;
             //       else deletesprite(i);
                    break;
            }
            break;

        case HANGLIGHT:
#if DUKE3D_GAMEVER >= GAMEVER_14
        case GENERICPOLE2:
#endif
            for(k=0;k<6;k++)
                {
                    // Fix: draw the random numbers in the DOS executable's (Watcom's) argument order.
                    int32_t r_zv = -(TRAND&4095)-(sprite[i].zvel>>2);
                    int32_t r_vel = (TRAND&63)+64;
                    int32_t r_ang = TRAND&2047;
                    int32_t r_pn = SCRAP1+(TRAND&15);
                    EGS(SECT,SX,SY,SZ-(8<<8),r_pn,-8,48,48,r_ang,r_vel,r_zv,i,5);
                }
            spritesound(GLASS_HEAVYBREAK,i);
            deletesprite(i);
            break;


        case FANSPRITE:
            PN = FANSPRITEBROKE;
            CS &= (65535-257);
            if( sector[SECT].floorpicnum == FANSHADOW )
                sector[SECT].floorpicnum = FANSHADOWBROKE;

#if DUKE3D_GAMEVER >= GAMEVER_14
            spritesound(GLASS_HEAVYBREAK,i);
#endif
            s = &sprite[i];
            for(j=0;j<16;j++) RANDOMSCRAP;

            break;
        case WATERFOUNTAIN:
        case WATERFOUNTAIN+1:
        case WATERFOUNTAIN+2:
        case WATERFOUNTAIN+3:
            PN = WATERFOUNTAINBROKE;
            spawn(i,TOILETWATER);
            break;
        case SATELITE:
        case FUELPOD:
        case SOLARPANNEL:
        case ANTENNA:
            if(sprite[sn].extra != *SCRIPT_PTR(actorscrptr[SHOTSPARK1]) )
            {
                for(j=0;j<15;j++)
                    {
                        // Fix: draw the random numbers in the DOS executable's (Watcom's) argument order.
                        int32_t r_zv = -(TRAND&511)-256;
                        int32_t r_vel = (TRAND&127)+64;
                        int32_t r_ang = TRAND&2047;
                        int32_t r_pn = SCRAP1+(TRAND&15);
                        EGS(SECT,SX,SY,sector[SECT].floorz-(12<<8)-(j<<9),r_pn,-8,64,64,r_ang,r_vel,r_zv,i,5);
                    }
                spawn(i,EXPLOSION2);
                deletesprite(i);
            }
            break;
        case BOTTLE1:
        case BOTTLE2:
        case BOTTLE3:
        case BOTTLE4:
        case BOTTLE5:
        case BOTTLE6:
        case BOTTLE8:
        case BOTTLE10:
        case BOTTLE11:
        case BOTTLE12:
        case BOTTLE13:
        case BOTTLE14:
        case BOTTLE15:
        case BOTTLE16:
        case BOTTLE17:
        case BOTTLE18:
        case BOTTLE19:
        case WATERFOUNTAINBROKE:
        case DOMELITE:
        case SUSHIPLATE1:
        case SUSHIPLATE2:
        case SUSHIPLATE3:
        case SUSHIPLATE4:
        case SUSHIPLATE5:
        case WAITTOBESEATED:
        case VASE:
        case STATUEFLASH:
        case STATUE:
            if(PN == BOTTLE10)
                lotsofmoney(&sprite[i],4+(TRAND&3));
            else if(PN == STATUE || PN == STATUEFLASH)
            {
                lotsofcolourglass(i,-1,40);
                spritesound(GLASS_HEAVYBREAK,i);
            }
            else if(PN == VASE)
                lotsofglass(i,-1,40);

            spritesound(GLASS_BREAKING,i);
            SA = TRAND&2047;
            lotsofglass(i,-1,8);
            deletesprite(i);
            break;
        case FETUS:
            PN = FETUSBROKE;
            spritesound(GLASS_BREAKING,i);
            lotsofglass(i,-1,10);
            break;
        case FETUSBROKE:
            for(j=0;j<48;j++)
            {
                shoot(i,BLOODSPLAT1);
                SA += 333;
            }
            spritesound(GLASS_HEAVYBREAK,i);
            spritesound(SQUISHED,i);
        case BOTTLE7:
            spritesound(GLASS_BREAKING,i);
            lotsofglass(i,-1,10);
            deletesprite(i);
            break;
        case HYDROPLANT:
            PN = BROKEHYDROPLANT;
            spritesound(GLASS_BREAKING,i);
            lotsofglass(i,-1,10);
            break;

        case FORCESPHERE:
            sprite[i].xrepeat = 0;
            hittype[OW].temp_data[0] = 32;
            hittype[OW].temp_data[1] = !hittype[OW].temp_data[1];
            hittype[OW].temp_data[2] ++;
            spawn(i,EXPLOSION2);
            break;

        case BROKEHYDROPLANT:
            if(CS&1)
            {
                spritesound(GLASS_BREAKING,i);
                SZ += 16<<8;
                CS = 0;
                lotsofglass(i,-1,5);
            }
            break;

        case TOILET:
            PN = TOILETBROKE;
            CS |= (TRAND&1)<<2;
            CS &= ~257;
            spawn(i,TOILETWATER);
            spritesound(GLASS_BREAKING,i);
            break;

        case STALL:
            PN = STALLBROKE;
            CS |= (TRAND&1)<<2;
            CS &= ~257;
            spawn(i,TOILETWATER);
            spritesound(GLASS_HEAVYBREAK,i);
            break;

        case HYDRENT:
            PN = BROKEFIREHYDRENT;
            spawn(i,TOILETWATER);

//            for(k=0;k<5;k++)
  //          {
    //            j = EGS(SECT,SX,SY,SZ-(TRAND%(48<<8)),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(sprite[i].zvel>>2),i,5);
      //          sprite[j].pal = 2;
        //    }
            spritesound(GLASS_HEAVYBREAK,i);
            break;

        case GRATE1:
            PN = BGRATE1;
            CS &= (65535-256-1);
            spritesound(VENT_BUST,i);
            break;

        case CIRCLEPANNEL:
            PN = CIRCLEPANNELBROKE;
            CS &= (65535-256-1);
            spritesound(VENT_BUST,i);
            break;
        case PANNEL1:
        case PANNEL2:
            PN = BPANNEL1;
            CS &= (65535-256-1);
            spritesound(VENT_BUST,i);
            break;
#if DUKE3D_GAMEVER >= GAMEVER_14
        case PANNEL3:
            PN = BPANNEL3;
            CS &= (65535-256-1);
            spritesound(VENT_BUST,i);
            break;
#endif
        case PIPE1:
        case PIPE2:
        case PIPE3:
        case PIPE4:
        case PIPE5:
        case PIPE6:
            switch(PN)
            {
                case PIPE1:PN=PIPE1B;break;
                case PIPE2:PN=PIPE2B;break;
                case PIPE3:PN=PIPE3B;break;
                case PIPE4:PN=PIPE4B;break;
                case PIPE5:PN=PIPE5B;break;
                case PIPE6:PN=PIPE6B;break;
            }

            j = spawn(i,STEAM);
            sprite[j].z = sector[SECT].floorz-(32<<8);
            break;

        case MONK:
        case LUKE:
        case INDY:
#if DUKE3D_GAMEVER >= GAMEVER_14
        case JURYGUY:
#endif
            spritesound(SLT,i);
            spawn(i,SHT);
        case SPACEMARINE:
            sprite[i].extra -= sprite[sn].extra;
            if(sprite[i].extra > 0) break;
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT1);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT2);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT3);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT4);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT1);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT2);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT3);
            SA = TRAND&2047;
            shoot(i,BLOODSPLAT4);
            guts(&sprite[i],JIBS1,1,myconnectindex);
            guts(&sprite[i],JIBS2,2,myconnectindex);
            guts(&sprite[i],JIBS3,3,myconnectindex);
            guts(&sprite[i],JIBS4,4,myconnectindex);
            guts(&sprite[i],JIBS5,1,myconnectindex);
            guts(&sprite[i],JIBS3,6,myconnectindex);
            sound(SQUISHED);
            deletesprite(i);
            break;
        case CHAIR1:
        case CHAIR2:
            PN = BROKENCHAIR;
            CS = 0;
            break;
        case CHAIR3:
        case MOVIECAMERA:
        case SCALE:
        case VACUUM:
        case CAMERALIGHT:
        case IVUNIT:
        case POT1:
        case POT2:
        case POT3:
#if DUKE3D_GAMEVER < GAMEVER_14
            spritesound(GLASS_HEAVYBREAK,i);
            s = &sprite[i];
            for(j=0;j<16;j++) RANDOMSCRAP;
            deletesprite(i);
            break;
        case CLOCK:
            PN = BROKENCLOCK;
            spritesound(GLASS_BREAKING,i);
            break;
#else
        case TRIPODCAMERA:
            spritesound(GLASS_HEAVYBREAK,i);
            s = &sprite[i];
            for(j=0;j<16;j++) RANDOMSCRAP;
            deletesprite(i);
            break;
#endif
        case PLAYERONWATER:
            i = OW;
        default:
#if DUKE3D_GAMEVER >= GAMEVER_14
            if( (sprite[i].cstat&16) && SHT == 0 && SLT == 0 && sprite[i].statnum == 0)
                break;
#endif

            if( ( sprite[sn].picnum == FREEZEBLAST || sprite[sn].owner != i ) && sprite[i].statnum != 4)
            {
                if( badguy(&sprite[i]) == 1)
                {
                    if(sprite[sn].picnum == RPG) sprite[sn].extra <<= 1;

                    if( (PN != DRONE) && (PN != ROTATEGUN) && (PN != COMMANDER) && (PN < GREENSLIME || PN > GREENSLIME+7) )
                        if(sprite[sn].picnum != FREEZEBLAST )
#if DUKE3D_GAMEVER >= GAMEVER_14
                            if( actortype[PN] == 0 )
#endif
                    {
                        j = spawn(sn,JIBS6);
#if DUKE3D_GAMEVER >= GAMEVER_14
                        if(sprite[sn].pal == 6)
                            sprite[j].pal = 6;
#endif
                        sprite[j].z += (4<<8);
                        sprite[j].xvel = 16;
                        sprite[j].xrepeat = sprite[j].yrepeat = 24;
                        sprite[j].ang += 32-(TRAND&63);
                    }

                    j = sprite[sn].owner;

                    if( j >= 0 && sprite[j].picnum == APLAYER && PN != ROTATEGUN && PN != DRONE )
                        if( ps[sprite[j].yvel].curr_weapon == SHOTGUN_WEAPON )
                    {
                        shoot(i,BLOODSPLAT3);
                        shoot(i,BLOODSPLAT1);
                        shoot(i,BLOODSPLAT2);
                        shoot(i,BLOODSPLAT4);
                    }

#if DUKE3D_GAMEVER < GAMEVER_14
                    if( PN != BOSS1 && PN != BOSS2 && PN != BOSS3 && PN != RECON && PN != ROTATEGUN )
                    {
                        SA = (sprite[sn].ang+1024)&2047;
#else
                    if( PN != TANK && PN != BOSS1 && PN != BOSS4 && PN != BOSS2 && PN != BOSS3 && PN != RECON && PN != ROTATEGUN )
                    {
                        if( (sprite[i].cstat&48) == 0 )
                            SA = (sprite[sn].ang+1024)&2047;
#endif
                        sprite[i].xvel = -(sprite[sn].extra<<2);
                        j = SECT;
                        pushmove(&SX,&SY,&SZ,&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
                        if(j != SECT && j >= 0 && j < MAXSECTORS)
                            changespritesect(i,j);
                    }

                    if(sprite[i].statnum == 2)
                    {
                        changespritestat(i,1);
#if DUKE3D_GAMEVER < GAMEVER_14
                        hittype[i].timetosleep = 0;
#else
                        hittype[i].timetosleep = SLEEPTIME;
#endif
                    }
                    if( ( RX < 24 || PN == SHARK) && sprite[sn].picnum == SHRINKSPARK) return;
                }

                if( sprite[i].statnum != 2 )
                {
#if DUKE3D_GAMEVER < GAMEVER_14
                    if( sprite[sn].picnum == FREEZEBLAST && freezerhurtowner == 0 && sprite[sn].owner == i )
#else
                    if( sprite[sn].picnum == FREEZEBLAST && ( (PN == APLAYER && sprite[i].pal == 1 ) || ( freezerhurtowner == 0 && sprite[sn].owner == i ) ) )
#endif
                        return;

                    hittype[i].picnum = sprite[sn].picnum;
                    hittype[i].extra += sprite[sn].extra;
                    hittype[i].ang = sprite[sn].ang;
                    hittype[i].owner = sprite[sn].owner;
                }

                if(sprite[i].statnum == 10)
                {
                    p = sprite[i].yvel;
                    if(ps[p].newowner >= 0)
                    {
                        ps[p].newowner = -1;
                        ps[p].posx = ps[p].oposx;
                        ps[p].posy = ps[p].oposy;
                        ps[p].posz = ps[p].oposz;
                        ps[p].ang = ps[p].oang;

                        updatesector(ps[p].posx,ps[p].posy,&ps[p].cursectnum);
                        setpal(&ps[p]);

                        j = headspritestat[1];
                        while(j >= 0)
                        {
                            if(sprite[j].picnum==CAMERA1) sprite[j].yvel = 0;
                            j = nextspritestat[j];
                        }
                    }

                    if( RX < 24 && sprite[sn].picnum == SHRINKSPARK)
                        return;

                    if( sprite[hittype[i].owner].picnum != APLAYER)
                        if(ud.player_skill >= 3)
                            sprite[sn].extra += (sprite[sn].extra>>1);
                }

            }
            break;
    }
}
#endif


void allignwarpelevators(void)
{
    short i, j;

    i = headspritestat[3];
    while(i >= 0)
    {
        if( SLT == 17 && SS > 16)
        {
            j = headspritestat[3];
            while(j >= 0)
            {
                if( (sprite[j].lotag) == 17 && i != j &&
                    (SHT) == (sprite[j].hitag) )
                {
                    sector[sprite[j].sectnum].floorz =
                        sector[SECT].floorz;
                    sector[sprite[j].sectnum].ceilingz =
                        sector[SECT].ceilingz;
                }

                j = nextspritestat[j];
            }
        }
        i = nextspritestat[i];
    }
}




#if DUKE3D_GAMEVER < GAMEVER_13D
// 1.1's cheatkeys (G11-S13): bit 23 toggles the aim mode, any key from bit 8
// up is debounced, the quick kick switches to the boot, and the inventory
// has the space suit as item 7 and the boots as item 8.
void cheatkeys(short snum)
{
    short i, k;
    char dainv;
    uint32_t sb_snum;
    int32_t j;
    struct player_struct *p;

    sb_snum = sync[snum].bits;
    p = &ps[snum];

    if(p->cheat_phase == 1) return;

    if( sb_snum&(1<<23) )
    {
        p->aim_mode = !p->aim_mode;
        if(p->aim_mode == 0)
            p->return_to_center = 9;
    }

    if( sb_snum < 0xff )
    {
        p->interface_toggle_flag = 0;
        return;
    }
    if(p->interface_toggle_flag) return;

    p->interface_toggle_flag = 1;

    if( sb_snum&(1<<21) )
    {
        KB_ClearKeyDown( sc_Pause );
        ud.pause_on = !ud.pause_on;
        if( ud.pause_on == 1 && sb_snum&(1<<5) ) ud.pause_on = 2;
        if(ud.pause_on)
        {
            MUSIC_Pause();
            FX_StopAllSounds();
        }
        else
        {
            if(MusicToggle) MUSIC_Continue();
            pub = NUMPAGES;
            pus = NUMPAGES;
        }
    }

    if(ud.pause_on) return;

    if(sprite[p->i].extra <= 0) return;

    if( sb_snum&(1<<30) && p->newowner == -1 )
    {
        switch(p->inven_icon)
        {
            case 4: sb_snum |= (1<<25);break;
            case 3: sb_snum |= (1<<24);break;
            case 5: sb_snum |= (1<<15);break;
            case 1: sb_snum |= (1<<16);break;
            case 2: sb_snum |= (1<<12);break;
        }
    }

    if( (sb_snum&(1<<12)) )
    {
        if(p->steroids_amount == 400 )
        {
            p->steroids_amount--;
            spritesound(DUKE_TAKEPILLS,p->i);
            p->inven_icon = 2;
            FTA(12,p);
        }
        return;
    }

    if(p->newowner == -1)
        if( sb_snum&(1<<20) || sb_snum&(1<<27) || p->refresh_inventory)
    {
        p->invdisptime = 26*2;

        if( sb_snum&(1<<27) ) k = 1;
        else k = 0;

        if(p->refresh_inventory) p->refresh_inventory = 0;
        dainv = p->inven_icon;

        i = 0;
        CHECKINV1:

        if(i < 9)
        {
            i++;

            switch(dainv)
            {
                case 4:
                    if(p->jetpack_amount > 0 && i > 1)
                        break;
                    if(k) dainv = 5;
                    else dainv = 3;
                    goto CHECKINV1;
                case 6:
                    if(p->scuba_amount > 0 && i > 1)
                        break;
                    if(k) dainv = 7;
                    else dainv = 5;
                    goto CHECKINV1;
                case 2:
                    if(p->steroids_amount > 0 && i > 1)
                        break;
                    if(k) dainv = 3;
                    else dainv = 1;
                    goto CHECKINV1;
                case 3:
                    if(p->holoduke_amount > 0 && i > 1)
                        break;
                    if(k) dainv = 4;
                    else dainv = 2;
                    goto CHECKINV1;
                case 1:
                    if(p->firstaid_amount > 0 && i > 1)
                        break;
                    if(k) dainv = 2;
                    else dainv = 8;
                    goto CHECKINV1;
                case 5:
                    if(p->heat_amount > 0 && i > 1)
                        break;
                    if(k) dainv = 6;
                    else dainv = 4;
                    goto CHECKINV1;
                case 0:
                case 7:
                    if(p->spacesuit_amount > 0 && i > 1)
                        break;
                    if(k) dainv = 8;
                    else dainv = 6;
                    goto CHECKINV1;
                case 8:
                    if(p->boot_amount > 0 && i > 1)
                        break;
                    if(k) dainv = 1;
                    else dainv = 7;
                    goto CHECKINV1;
            }
        }
        else dainv = 0;
        p->inven_icon = dainv;

        switch(dainv)
        {
            case 1: FTA(3,p);break;
            case 2: FTA(90,p);break;
            case 3: FTA(91,p);break;
            case 4: FTA(88,p);break;
            case 5: FTA(101,p);break;
            case 6: FTA(89,p);break;
            case 7: FTA(87,p);break;
            case 8: FTA(6,p);break;
        }
    }

    if(p->last_pissed_time <= (26*218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && sprite[p->i].xrepeat > 32 && p->access_incs == 0 && p->knee_incs == 0 )
    {
        if(  ( p->weapon_pos == 0 || ( p->holster_weapon && p->weapon_pos == -9 ) ) )
        {
            j = ( (sb_snum&(15<<8))>>8 ) - 1;

            // 1.1 leaves k at 0 unless it looks for a live pipe bomb.
            k = 0;

            if( j == HANDBOMB_WEAPON && p->ammo_amount[HANDBOMB_WEAPON] == 0 )
            {
                k = headspritestat[1];
                while(k >= 0)
                {
                    if( sprite[k].picnum == HEAVYHBOMB && sprite[k].owner == p->i )
                    {
                        p->gotweapon[HANDBOMB_WEAPON] = 1;
                        j = HANDREMOTE_WEAPON;
                        break;
                    }
                    k = nextspritestat[k];
                }
            }

            if(p->holster_weapon)
            {
                sb_snum |= 1<<19;
                p->weapon_pos = -9;
            }
            else if( p->gotweapon[j] && p->curr_weapon != j ) switch(j)
            {
                case KNEE_WEAPON:
                    addweapon( p, KNEE_WEAPON );
                    break;
                case PISTOL_WEAPON:
                case SHOTGUN_WEAPON:
                case CHAINGUN_WEAPON:
                case RPG_WEAPON:
                case SHRINKER_WEAPON:
                case DEVISTATOR_WEAPON: // the flamethrower
                    if( p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
                    {
                        p->show_empty_weapon = 32;
                        p->last_full_weapon = p->curr_weapon;
                    }
                    addweapon( p, j );
                    break;
                case HANDREMOTE_WEAPON:
                    if(k >= 0) // Found in list of [1]'s
                    {
                        p->curr_weapon = HANDREMOTE_WEAPON;
                        p->last_weapon = -1;
                        p->weapon_pos = 10;
                    }
                    break;
                case HANDBOMB_WEAPON:
                    if( p->ammo_amount[HANDBOMB_WEAPON] > 0 && p->gotweapon[HANDBOMB_WEAPON] )
                        addweapon( p, HANDBOMB_WEAPON );
                    break;
                case TRIPBOMB_WEAPON:
                    if( p->ammo_amount[TRIPBOMB_WEAPON] > 0 && p->gotweapon[TRIPBOMB_WEAPON] )
                        addweapon( p, TRIPBOMB_WEAPON );
                    break;
            }
        }

        // The quick kick swaps to the boot; doincrements swaps back.
        if( sb_snum&(1<<22) )
        {
            if(p->holster_weapon == 0)
            {
                if(p->curr_weapon == KNEE_WEAPON)
                    p->kickback_pic = 1;
                else
                {
                    p->last_full_weapon = p->curr_weapon;
                    p->quick_kick = 24;
                    FTA(80,p);
                    addweapon( p, KNEE_WEAPON );
                }
            }
            else
            {
                p->weapon_pos = -9;
                sb_snum |= 1<<19;
            }
        }

        if( sb_snum&(1<<19) && p->curr_weapon > KNEE_WEAPON && p->somethingonplayer == -1 )
        {
            if(p->holster_weapon == 0 && p->weapon_pos == 0)
            {
                p->holster_weapon = 1;
                p->weapon_pos = -1;
                FTA(73,p);
            }
            else if(p->holster_weapon == 1 && p->weapon_pos == -9)
            {
                p->holster_weapon = 0;
                p->weapon_pos = 10;
                FTA(74,p);
            }
        }
    }

    if( sb_snum&(1<<24) && p->newowner == -1 )
    {
        if( p->holoduke_on == -1 )
        {

            if( p->holoduke_amount > 0 )
            {
                p->inven_icon = 3;

                p->holoduke_on = i =
                    EGS(p->cursectnum,
                    p->posx,
                    p->posy,
                    p->posz+(30<<8),APLAYER,-64,0,0,p->ang,0,0,-1,10);
                T4 = T5 = 0;
                SP = snum;
                sprite[i].extra = 0;
                FTA(47,p);
            }
            else FTA(49,p);
            spritesound(TELEPORTER,p->holoduke_on);

        }
        else
        {
            spritesound(TELEPORTER,p->holoduke_on);
            p->holoduke_on = -1;
            FTA(48,p);
        }
    }

    if( sb_snum&(1<<15) && p->heat_amount > 0 && p->cheat_phase == 0 )
    {
        p->heat_on = !p->heat_on;
        setpal(p);
        p->inven_icon = 5;
        spritesound(NITEVISION_ONOFF,p->i);
        FTA(106+(!p->heat_on),p);
    }

    if( sb_snum&(1<<16) )
    {
        if( p->firstaid_amount > 0 && sprite[p->i].extra < max_player_health )
        {
            j = max_player_health-sprite[p->i].extra;

            if(p->firstaid_amount >= j)
            {
                p->firstaid_amount -= j;
                sprite[p->i].extra = max_player_health;
                p->inven_icon = 1;
            }
            else
            {
                sprite[p->i].extra += p->firstaid_amount;
                p->firstaid_amount = 0;
                checkavailinven(p);
            }
            spritesound(DUKE_USEMEDKIT,p->i);
        }
    }

    if( sb_snum&(1<<25) && p->newowner == -1)
    {
        if( p->jetpack_amount > 0 )
        {
            p->jetpack_on = !p->jetpack_on;
            if(p->jetpack_on)
            {
                p->inven_icon = 4;
                spritesound(DUKE_JETPACK_ON,p->i);
                FTA(52,p);
            }
            else
            {
                p->hard_landing = 0;
                p->poszv = 0;
                spritesound(DUKE_JETPACK_OFF,p->i);
                stopsound(DUKE_JETPACK_IDLE);
                stopsound(DUKE_JETPACK_ON);
                FTA(53,p);
            }
        }
        else FTA(50,p);
    }

    if(sb_snum&(1<<28) && p->one_eighty_count == 0)
        p->one_eighty_count = -1024;
}
#else
void cheatkeys(short snum)
{
    short i, k;
    char dainv;
    uint32_t sb_snum;
    int32_t j;
    struct player_struct *p;

    sb_snum = sync[snum].bits;
    p = &ps[snum];

    if(p->cheat_phase == 1) return;

    i = p->aim_mode;
    p->aim_mode = (sb_snum>>23)&1;
    if(p->aim_mode < i)
        p->return_to_center = 9;

    if( (sb_snum&(1<<22)) && p->quick_kick == 0)
#if DUKE3D_GAMEVER >= GAMEVER_14
        if( p->curr_weapon != KNEE_WEAPON || p->kickback_pic == 0 )
#endif
    {
        p->quick_kick = 14;
        FTA(80,p);
    }

    if( !(sb_snum&((15<<8)|(1<<12)|(1<<15)|(1<<16)|(1<<22)|(1<<19)|(1<<20)|(1<<21)|(1<<24)|(1<<25)|(1<<27)|(1<<28)|(1<<29)|(1<<30)|(1<<31))) )
        p->interface_toggle_flag = 0;
#if DUKE3D_GAMEVER < GAMEVER_14
    else if(p->interface_toggle_flag == 0)
#else
    else if(p->interface_toggle_flag == 0 && ( sb_snum&(1<<17) ) == 0)
#endif
    {
        p->interface_toggle_flag = 1;

        if( sb_snum&(1<<21) )
        {
            KB_ClearKeyDown( sc_Pause );
            ud.pause_on = !ud.pause_on;
            if( ud.pause_on == 1 && sb_snum&(1<<5) ) ud.pause_on = 2;
            if(ud.pause_on)
            {
                MUSIC_Pause();
                FX_StopAllSounds();
                clearsoundlocks();
            }
            else
            {
                if(MusicToggle) MUSIC_Continue();
                pub = NUMPAGES;
                pus = NUMPAGES;
            }
        }

        if(ud.pause_on) return;
        
        if(sprite[p->i].extra <= 0) return;

        if( sb_snum&(1<<30) && p->newowner == -1 )
        {
            switch(p->inven_icon)
            {
                case 4: sb_snum |= (1<<25);break;
                case 3: sb_snum |= (1<<24);break;
                case 5: sb_snum |= (1<<15);break;
                case 1: sb_snum |= (1<<16);break;
                case 2: sb_snum |= (1<<12);break;
            }
        }

#if DUKE3D_GAMEVER >= GAMEVER_14
        if( sb_snum&(1<<15) && p->heat_amount > 0 )
        {
            p->heat_on = !p->heat_on;
            setpal(p);
            p->inven_icon = 5;
            spritesound(NITEVISION_ONOFF,p->i);
            FTA(106+(!p->heat_on),p);
        }
#endif

        if( (sb_snum&(1<<12)) )
        {
            if(p->steroids_amount == 400 )
            {
                p->steroids_amount--;
                spritesound(DUKE_TAKEPILLS,p->i);
                p->inven_icon = 2;
                FTA(12,p);
            }
            return;
        }

        if(p->newowner == -1)
            if( sb_snum&(1<<20) || sb_snum&(1<<27) || p->refresh_inventory)
        {
            p->invdisptime = 26*2;

            if( sb_snum&(1<<27) ) k = 1;
            else k = 0;

            if(p->refresh_inventory) p->refresh_inventory = 0;
            dainv = p->inven_icon;

            i = 0;
            CHECKINV1:

            if(i < 9)
            {
                i++;

                switch(dainv)
                {
                    case 4:
                        if(p->jetpack_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 5;
                        else dainv = 3;
                        goto CHECKINV1;
                    case 6:
                        if(p->scuba_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 7;
                        else dainv = 5;
                        goto CHECKINV1;
                    case 2:
                        if(p->steroids_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 3;
                        else dainv = 1;
                        goto CHECKINV1;
                    case 3:
                        if(p->holoduke_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 4;
                        else dainv = 2;
                        goto CHECKINV1;
                    case 0:
                    case 1:
                        if(p->firstaid_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 2;
                        else dainv = 7;
                        goto CHECKINV1;
                    case 5:
                        if(p->heat_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 6;
                        else dainv = 4;
                        goto CHECKINV1;
                    case 7:
                        if(p->boot_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 1;
                        else dainv = 6;
                        goto CHECKINV1;
                }
            }
            else dainv = 0;
            p->inven_icon = dainv;

            switch(dainv)
            {
                case 1: FTA(3,p);break;
                case 2: FTA(90,p);break;
                case 3: FTA(91,p);break;
                case 4: FTA(88,p);break;
                case 5: FTA(101,p);break;
                case 6: FTA(89,p);break;
                case 7: FTA(6,p);break;
            }
        }

        j = ( (sb_snum&(15<<8))>>8 ) - 1;

#if DUKE3D_GAMEVER >= GAMEVER_14
        if( j > 0 && p->kickback_pic > 0)
            p->wantweaponfire = j;
#endif

        if(p->last_pissed_time <= (26*218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && sprite[p->i].xrepeat > 32 && p->access_incs == 0 && p->knee_incs == 0 )
        {
            if(  ( p->weapon_pos == 0 || ( p->holster_weapon && p->weapon_pos == -9 ) ) )
            {
                if(j == 10 || j == 11)
                {
                    k = p->curr_weapon;
                    j = ( j == 10 ? -1 : 1 );
                    i = 0;

#if DUKE3D_GAMEVER < GAMEVER_14
                    // 1.3D has no expander to cycle through.
                    while( k >= 0 && k < 10 )
                    {
                        k += j;

                        if(k == -1) k = 9;
                        else if(k == 10) k = 0;

                        if( p->gotweapon[k] && p->ammo_amount[k] > 0 )
                        {
                            j = k;
                            break;
                        }

                        i++;
                        if(i == 10)
                        {
                            addweapon( p, KNEE_WEAPON );
                            break;
                        }
                    }
#else
                    while( ( k >= 0 && k < 10 ) || ( k == GROW_WEAPON && (p->subweapon&(1<<GROW_WEAPON) ) ) )
                    {
                        if(k == GROW_WEAPON)
                        {
                            if(j == -1)
                                k = 5;
                            else k = 7;

                        }
                        else
                        {
                           k += j;
                           if( k == 6 && p->subweapon&(1<<GROW_WEAPON) )
                               k = GROW_WEAPON;
                        }

                        if(k == -1) k = 9;
                        else if(k == 10) k = 0;

                        if( p->gotweapon[k] && p->ammo_amount[k] > 0 )
                        {
                            if( k == SHRINKER_WEAPON && p->subweapon&(1<<GROW_WEAPON) )
                                k = GROW_WEAPON;
                            j = k;
                            break;
                        }
                        else
                            if(k == GROW_WEAPON && p->ammo_amount[GROW_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[SHRINKER_WEAPON] > 0)
                        {
                            j = SHRINKER_WEAPON;
                            p->subweapon &= ~(1<<GROW_WEAPON);
                            break;
                        }
                        else
                            if(k == SHRINKER_WEAPON && p->ammo_amount[SHRINKER_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[GROW_WEAPON] > 0)
                        {
                            j = GROW_WEAPON;
                            p->subweapon |= (1<<GROW_WEAPON);
                            break;
                        }

                        i++;
                        if(i == 10)
                        {
                            addweapon( p, KNEE_WEAPON );
                            break;
                        }
                    }
#endif
                }

                k = -1;


                if( j == HANDBOMB_WEAPON && p->ammo_amount[HANDBOMB_WEAPON] == 0 )
                {
                    k = headspritestat[1];
                    while(k >= 0)
                    {
                        if( sprite[k].picnum == HEAVYHBOMB && sprite[k].owner == p->i )
                        {
                            p->gotweapon[HANDBOMB_WEAPON] = 1;
                            j = HANDREMOTE_WEAPON;
                            break;
                        }
                        k = nextspritestat[k];
                    }
                }

#if DUKE3D_GAMEVER >= GAMEVER_14
                if(j == SHRINKER_WEAPON)
                {
                    if(screenpeek == snum) pus = NUMPAGES;

                    if( p->curr_weapon != GROW_WEAPON && p->curr_weapon != SHRINKER_WEAPON )
                    {
                        if( p->ammo_amount[GROW_WEAPON] > 0 )
                        {
                            if( (p->subweapon&(1<<GROW_WEAPON)) == (1<<GROW_WEAPON) )
                                j = GROW_WEAPON;
                            else if(p->ammo_amount[SHRINKER_WEAPON] == 0)
                            {
                                j = GROW_WEAPON;
                                p->subweapon |= (1<<GROW_WEAPON);
                            }
                        }
                        else if( p->ammo_amount[SHRINKER_WEAPON] > 0 )
                            p->subweapon &= ~(1<<GROW_WEAPON);
                    }
                    else if( p->curr_weapon == SHRINKER_WEAPON )
                    {
                        p->subweapon |= (1<<GROW_WEAPON);
                        j = GROW_WEAPON;
                    }
                    else
                        p->subweapon &= ~(1<<GROW_WEAPON);
                }
#endif

                if(p->holster_weapon)
                {
                    sb_snum |= 1<<19;
                    p->weapon_pos = -9;
                }
                else if( p->gotweapon[j] && p->curr_weapon != j ) switch(j)
                {
                    case KNEE_WEAPON:
                        addweapon( p, KNEE_WEAPON );
                        break;
                    case PISTOL_WEAPON:
                        if ( p->ammo_amount[PISTOL_WEAPON] == 0 )
                            if(p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, PISTOL_WEAPON );
                        break;
                    case SHOTGUN_WEAPON:
                        if( p->ammo_amount[SHOTGUN_WEAPON] == 0 && p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, SHOTGUN_WEAPON);
                        break;
                    case CHAINGUN_WEAPON:
                        if( p->ammo_amount[CHAINGUN_WEAPON] == 0 && p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, CHAINGUN_WEAPON);
                        break;
                    case RPG_WEAPON:
                        if( p->ammo_amount[RPG_WEAPON] == 0 )
                            if(p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, RPG_WEAPON );
                        break;
                    case DEVISTATOR_WEAPON:
                        if( p->ammo_amount[DEVISTATOR_WEAPON] == 0 && p->show_empty_weapon == 0 )
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, DEVISTATOR_WEAPON );
                        break;
                    case FREEZE_WEAPON:
                        if( p->ammo_amount[FREEZE_WEAPON] == 0 && p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, FREEZE_WEAPON );
                        break;
                    case GROW_WEAPON:
                    case SHRINKER_WEAPON:

                        if( p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
                        {
                            p->show_empty_weapon = 32;
                            p->last_full_weapon = p->curr_weapon;
                        }

                        addweapon(p, j);
                        break;
                    case HANDREMOTE_WEAPON:
                        if(k >= 0) // Found in list of [1]'s
                        {
                            p->curr_weapon = HANDREMOTE_WEAPON;
                            p->last_weapon = -1;
                            p->weapon_pos = 10;
                        }
                        break;
                    case HANDBOMB_WEAPON:
                        if( p->ammo_amount[HANDBOMB_WEAPON] > 0 && p->gotweapon[HANDBOMB_WEAPON] )
                            addweapon( p, HANDBOMB_WEAPON );
                        break;
                    case TRIPBOMB_WEAPON:
                        if( p->ammo_amount[TRIPBOMB_WEAPON] > 0 && p->gotweapon[TRIPBOMB_WEAPON] )
                            addweapon( p, TRIPBOMB_WEAPON );
                        break;
                }
            }

            if( sb_snum&(1<<19) )
            {
                if( p->curr_weapon > KNEE_WEAPON )
                {
                    if(p->holster_weapon == 0 && p->weapon_pos == 0)
                    {
                        p->holster_weapon = 1;
                        p->weapon_pos = -1;
                        FTA(73,p);
                    }
                    else if(p->holster_weapon == 1 && p->weapon_pos == -9)
                    {
                        p->holster_weapon = 0;
                        p->weapon_pos = 10;
                        FTA(74,p);
                    }
                }
            }
        }

        if( sb_snum&(1<<24) && p->newowner == -1 )
        {
            if( p->holoduke_on == -1 )
            {

                if( p->holoduke_amount > 0 )
                {
                    p->inven_icon = 3;

                    p->holoduke_on = i =
                        EGS(p->cursectnum,
                        p->posx,
                        p->posy,
                        p->posz+(30<<8),APLAYER,-64,0,0,p->ang,0,0,-1,10);
                    T4 = T5 = 0;
                    SP = snum;
                    sprite[i].extra = 0;
                    FTA(47,p);
                }
                else FTA(49,p);
                spritesound(TELEPORTER,p->holoduke_on);

            }
            else
            {
                spritesound(TELEPORTER,p->holoduke_on);
                p->holoduke_on = -1;
                FTA(48,p);
            }
        }

#if DUKE3D_GAMEVER < GAMEVER_14
        // 1.3D toggles the goggles here, and not while a cheat is typed.
        if( sb_snum&(1<<15) && p->heat_amount > 0 && p->cheat_phase == 0 )
        {
            p->heat_on = !p->heat_on;
            setpal(p);
            p->inven_icon = 5;
            spritesound(NITEVISION_ONOFF,p->i);
            FTA(106+(!p->heat_on),p);
        }
#endif

        if( sb_snum&(1<<16) )
        {
            if( p->firstaid_amount > 0 && sprite[p->i].extra < max_player_health )
            {
                j = max_player_health-sprite[p->i].extra;

#if DUKE3D_GAMEVER < GAMEVER_14
                if(p->firstaid_amount >= j)
#else
                if(p->firstaid_amount > j)
#endif
                {
                    p->firstaid_amount -= j;
                    sprite[p->i].extra = max_player_health;
                    p->inven_icon = 1;
                }
                else
                {
                    sprite[p->i].extra += p->firstaid_amount;
                    p->firstaid_amount = 0;
                    checkavailinven(p);
                }
                spritesound(DUKE_USEMEDKIT,p->i);
            }
        }

        if( sb_snum&(1<<25) && p->newowner == -1)
        {
            if( p->jetpack_amount > 0 )
            {
                p->jetpack_on = !p->jetpack_on;
                if(p->jetpack_on)
                {
                    p->inven_icon = 4;
                    if(p->scream_voice > FX_Ok)
                    {
                        FX_StopSound(p->scream_voice);
                        testcallback(DUKE_SCREAM);
                        p->scream_voice = FX_Ok;
                    }

                    spritesound(DUKE_JETPACK_ON,p->i);

                    FTA(52,p);
                }
                else
                {
                    p->hard_landing = 0;
                    p->poszv = 0;
                    spritesound(DUKE_JETPACK_OFF,p->i);
                    stopsound(DUKE_JETPACK_IDLE);
                    stopsound(DUKE_JETPACK_ON);
                    FTA(53,p);
                }
            }
            else FTA(50,p);
        }

        if(sb_snum&(1<<28) && p->one_eighty_count == 0)
            p->one_eighty_count = -1024;
    }
}
#endif

void checksectors(short snum)
{
    int32_t i = -1,oldz;
    struct player_struct *p;
    short j,hitscanwall;

    p = &ps[snum];

    switch(sector[p->cursectnum].lotag)
    {

        case 32767:
            sector[p->cursectnum].lotag = 0;
            FTA(9,p);
            p->secret_rooms++;
            return;
        case -1:
            for(i=connecthead;i>=0;i=connectpoint2[i])
                ps[i].gm = MODE_EOL;
            sector[p->cursectnum].lotag = 0;
            if(ud.from_bonus)
            {
                ud.level_number = ud.from_bonus;
                ud.m_level_number = ud.level_number;
                ud.from_bonus = 0;
            }
            else
            {
                ud.level_number++;
#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 doesn't wrap; main ends the episode at level 5
                if( (ud.volume_number && ud.level_number > 10 ) || ud.level_number > 5 )
                    ud.level_number = 0;
#endif
                ud.m_level_number = ud.level_number;
            }
            return;
        case -2:
            sector[p->cursectnum].lotag = 0;
            p->timebeforeexit = 26*8;
            p->customexitsound = sector[p->cursectnum].hitag;
            return;
        default:
#if DUKE3D_GAMEVER < GAMEVER_14
            if(sector[p->cursectnum].lotag > 9999)
#else
            if(sector[p->cursectnum].lotag >= 10000 && sector[p->cursectnum].lotag < 16383)
#endif
            {
#if DUKE3D_GAMEVER < GAMEVER_13D
                if(snum == screenpeek || ud.coop != 0 )
#else
                if(snum == screenpeek || ud.coop == 1 )
#endif
                    spritesound(sector[p->cursectnum].lotag-10000,p->i);
                sector[p->cursectnum].lotag = 0;
            }
            break;

    }

    //After this point the the player effects the map with space

    if(p->gm&MODE_TYPE || sprite[p->i].extra <= 0) return;

    if( ud.cashman && sync[snum].bits&(1<<29) )
        lotsofmoney(&sprite[p->i],2);

    if(p->newowner >= 0)
    {
#if DUKE3D_GAMEVER < GAMEVER_13D // 1.1 leaves the camera only on forward motion
        if( klabs(sync[snum].fvel) > 768 )
#else
        if( klabs(sync[snum].svel) > 768 || klabs(sync[snum].fvel) > 768 )
#endif
        {
            i = -1;
            goto CLEARCAMERAS;
        }
    }

    if( !(sync[snum].bits&(1<<29)) && !(sync[snum].bits&(1<<31)))
        p->toggle_key_flag = 0;

    else if(!p->toggle_key_flag)
    {

        if( (sync[snum].bits&(1<<31)) )
        {
            if( p->newowner >= 0 )
            {
                i = -1;
                goto CLEARCAMERAS;
            }
            return;
        }

        neartagsprite = -1;
        p->toggle_key_flag = 1;
        hitscanwall = -1;

        i = hitawall(p,&hitscanwall);

#if DUKE3D_GAMEVER < GAMEVER_14
        if(hitscanwall >= 0 && wall[hitscanwall].overpicnum == MIRROR)
#else
        if(i < 1280 && hitscanwall >= 0 && wall[hitscanwall].overpicnum == MIRROR)
#endif
            if( wall[hitscanwall].lotag > 0 && Sound[wall[hitscanwall].lotag].num == 0 && snum == screenpeek)
        {
            spritesound(wall[hitscanwall].lotag,p->i);
            return;
        }

#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 lets the use action reach past masked walls
        if(hitscanwall >= 0 && (wall[hitscanwall].cstat&16) )
            switch(wall[hitscanwall].overpicnum)
        {
            default:
                if(wall[hitscanwall].lotag)
                    return;
        }
#endif

        if(p->newowner >= 0)
            neartag(p->oposx,p->oposy,p->oposz,sprite[p->i].sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,1);
        else
        {
            neartag(p->posx,p->posy,p->posz,sprite[p->i].sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,1);
            if(neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
                neartag(p->posx,p->posy,p->posz+(8<<8),sprite[p->i].sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,1);
            if(neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
                neartag(p->posx,p->posy,p->posz+(16<<8),sprite[p->i].sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,1);
#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 tries three times only
            if(neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
            {
                neartag(p->posx,p->posy,p->posz+(16<<8),sprite[p->i].sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,3);
                if(neartagsprite >= 0)
                {
                    switch(sprite[neartagsprite].picnum)
                    {
                        case FEM1:
                        case FEM2:
                        case FEM3:
                        case FEM4:
                        case FEM5:
                        case FEM6:
                        case FEM7:
                        case FEM8:
                        case FEM9:
#if DUKE3D_GAMEVER >= GAMEVER_14
                        case FEM10:
#endif
                        case PODFEM1:
                        case NAKED1:
                        case STATUE:
#if DUKE3D_GAMEVER >= GAMEVER_14
                        case TOUGHGAL:
#endif
                            return;
                    }
                }

                neartagsprite = -1;
                neartagwall = -1;
                neartagsector = -1;
            }
#endif
        }

        if(p->newowner == -1 && neartagsprite == -1 && neartagsector == -1 && neartagwall == -1 )
            if( isanunderoperator(sector[sprite[p->i].sectnum].lotag) )
                neartagsector = sprite[p->i].sectnum;

        if( neartagsector >= 0 && (sector[neartagsector].lotag&16384) )
            return;

        if( neartagsprite == -1 && neartagwall == -1)
            if(sector[p->cursectnum].lotag == 2 )
            {
                oldz = hitasprite(p->i,&neartagsprite);
                if(oldz > 1280) neartagsprite = -1;
            }

        if(neartagsprite >= 0)
        {
            if( checkhitswitch(snum,neartagsprite,1) ) return;

            switch(sprite[neartagsprite].picnum)
            {
                case TOILET:
                case STALL:
                    if(p->last_pissed_time == 0)
                    {
                        if(ud.lockout == 0) spritesound(DUKE_URINATE,p->i);

                        p->last_pissed_time = 26*220;
                        p->transporter_hold = 29*2;
                        if(p->holster_weapon == 0)
                        {
                            p->holster_weapon = 1;
                            p->weapon_pos = -1;
                        }
                        if(sprite[p->i].extra <= (max_player_health-(max_player_health/10) ) )
                        {
                            sprite[p->i].extra += max_player_health/10;
                            p->last_extra = sprite[p->i].extra;
                        }
                        else if(sprite[p->i].extra < max_player_health )
                             sprite[p->i].extra = max_player_health;
                    }
                    else if(Sound[FLUSH_TOILET].num == 0)
                        spritesound(FLUSH_TOILET,p->i);
                    return;

                case NUKEBUTTON:

                    j = -1; // -1: 1.0's hitscan leaves this unwritten on a miss (DOS read stack garbage)
                    hitawall(p,&j);
                    if(j >= 0 && wall[j].overpicnum == 0)
                        if(hittype[neartagsprite].temp_data[0] == 0)
                    {
                        hittype[neartagsprite].temp_data[0] = 1;
                        sprite[neartagsprite].owner = p->i;
                        p->buttonpalette = sprite[neartagsprite].pal;
#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1's secret exit is fixed (G11-G30)
                        if(p->buttonpalette)
                            ud.secretlevel = sprite[neartagsprite].lotag;
                        else ud.secretlevel = 0;
#endif
                    }
                    return;
                case WATERFOUNTAIN:
                    if(hittype[neartagsprite].temp_data[0] != 1)
                    {
                        hittype[neartagsprite].temp_data[0] = 1;
                        sprite[neartagsprite].owner = p->i;

#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 gives no health here
                        if(sprite[p->i].extra < max_player_health)
                        {
                            sprite[p->i].extra++;
                            spritesound(DUKE_DRINKING,p->i);
                        }
#endif
                    }
                    return;
                case PLUG:
                    spritesound(SHORT_CIRCUIT,p->i);
                    sprite[p->i].extra -= 2+(TRAND&3);
                    p->pals[0] = 48;
                    p->pals[1] = 48;
                    p->pals[2] = 64;
                    p->pals_time = 32;
                    break;
#if DUKE3D_GAMEVER < GAMEVER_13D
                case PODFEM1:
                    if(ud.lockout == 0 && Sound[KILLME].num == 0)
                        spritesound(KILLME,neartagsprite);
                    break;
                // 1.1 lets the player tip the strippers (G11-S15).
                case FEM1:
                case FEM2:
                case FEM3:
                case FEM4:
                    if(p->tipincs == 0)
                    {
                        p->tipincs = 26;
                        hittype[neartagsprite].temp_data[0] = 1;
                        if(TRAND&1)
                            spritesound(DUKE_TIP1,p->i);
                        else spritesound(DUKE_TIP2,p->i);
                    }
                    return;
                case VIEWSCREEN:
#else
                case VIEWSCREEN:
                case VIEWSCREEN2:
#endif
                    {
                        i = headspritestat[1];

                        while(i >= 0)
                        {
                            if( PN == CAMERA1 && SP == 0 && sprite[neartagsprite].hitag == SLT )
                            {
                                SP = 1; //Using this camera
#if DUKE3D_GAMEVER < GAMEVER_13D
                                spritesound(MONITOR_ACTIVE,p->i);
#else
                                spritesound(MONITOR_ACTIVE,neartagsprite);
#endif

                                sprite[neartagsprite].owner = i;
                                sprite[neartagsprite].yvel = 1;


                                j = p->cursectnum;
                                p->cursectnum = SECT;
                                setpal(p);
                                p->cursectnum = j;

                                // parallaxtype = 2;
                                p->newowner = i;
                                return;
                            }
                            i = nextspritestat[i];
                       }
                    }

                    CLEARCAMERAS:

                    if(i < 0)
                    {
                        p->posx = p->oposx;
                        p->posy = p->oposy;
                        p->posz = p->oposz;
                        p->ang = p->oang;
                        p->newowner = -1;

#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 keeps the cursectnum it had
                        updatesector(p->posx,p->posy,&p->cursectnum);
#endif
                        setpal(p);


                        i = headspritestat[1];
                        while(i >= 0)
                        {
                            if(PN==CAMERA1) SP = 0;
                            i = nextspritestat[i];
                        }
                    }
                    else if(p->newowner >= 0)
                        p->newowner = -1;

                    if( KB_KeyPressed(sc_Escape) )
                        KB_ClearKeyDown(sc_Escape);

                    return;
            }
        }

        if( (sync[snum].bits&(1<<29)) == 0 ) return;
        else if(p->newowner >= 0) { i = -1; goto CLEARCAMERAS; }

        if(neartagwall == -1 && neartagsector == -1 && neartagsprite == -1)
            if( klabs(hits(p->i)) < 512 )
        {
            if( (TRAND&255) < 16 )
                spritesound(DUKE_SEARCH2,p->i);
            else spritesound(DUKE_SEARCH,p->i);
            return;
        }

        if( neartagwall >= 0 )
        {
            if( wall[neartagwall].lotag > 0 && isadoorwall(wall[neartagwall].picnum) )
            {
#if DUKE3D_GAMEVER >= GAMEVER_13D // 1.1 doesn't check that the door wall was the one aimed at
                if(hitscanwall == neartagwall || hitscanwall == -1)
#endif
                    checkhitswitch(snum,neartagwall,0);
                return;
            }
            else if(p->newowner >= 0)
            {
                i = -1;
                goto CLEARCAMERAS;
            }
        }

        if( neartagsector >= 0 && (sector[neartagsector].lotag&16384) == 0 && isanearoperator(sector[neartagsector].lotag) )
        {
            i = headspritesect[neartagsector];
            while(i >= 0)
            {
                if( PN == ACTIVATOR || PN == MASTERSWITCH )
                    return;
                i = nextspritesect[i];
            }
            operatesectors(neartagsector,p->i);
        }
        else if( (sector[sprite[p->i].sectnum].lotag&16384) == 0 )
        {
            if( isanunderoperator(sector[sprite[p->i].sectnum].lotag) )
            {
                i = headspritesect[sprite[p->i].sectnum];
                while(i >= 0)
                {
                    if(PN == ACTIVATOR || PN == MASTERSWITCH) return;
                    i = nextspritesect[i];
                }
                operatesectors(sprite[p->i].sectnum,p->i);
            }
            else checkhitswitch(snum,neartagwall,0);
        }
    }
}




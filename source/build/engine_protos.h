#include <stdint.h>
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

*/
//-------------------------------------------------------------------------

/* Prototypes for the build engine */

#if 0
/* game.c */
extern void initsb(char dadigistat, char damusistat, int32_t dasamplerate, char danumspeakers, char dabytespersample, char daintspersec, char daquality);
extern void uninitsb(void);
extern int loadsong(char *filename);
extern void musicon(void);
extern void musicoff(void);
extern void wsayfollow(char *dafilename, int32_t dafreq, int32_t davol, int32_t *daxplc, int32_t *dayplc, char followstat);
extern void wsay(char *dafilename, int32_t dafreq, int32_t volume1, int32_t volume2);
extern void preparesndbuf(void);
extern void setears(int32_t daposx, int32_t daposy, int32_t daxvect, int32_t dayvect);
extern void timerhandler(void);
extern void keyhandler(void);
extern void initlava(void);
extern void movelava(char *dapic);
extern void drawtilebackground(int32_t thex, int32_t they, short tilenum, signed char shade, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, char dapalnum);
extern void printext(int32_t x, int32_t y, char *buffer, short tilenum, char invisiblecol);
extern void drawstatusflytime(short snum);
extern void drawstatusbar(short snum);
extern void setup3dscreen(void);
extern void setinterpolation(int32_t *posptr);
extern void stopinterpolation(int32_t *posptr);
extern void updateinterpolations(void);
extern void restoreinterpolations(void);
extern void searchmap(short startsector);
extern void prepareboard(char *daboardfilename);
extern int32_t changehealth(short snum, short deltahealth);
extern void changenumbombs(short snum, short deltanumbombs);
extern void changenummissiles(short snum, short deltanummissiles);
extern void changenumgrabbers(short snum, short deltanumgrabbers);
extern void findrandomspot(int32_t *x, int32_t *y, short *sectnum);
extern void operatesector(short dasector);
extern void shootgun(short snum, int32_t x, int32_t y, int32_t z, short daang, int32_t dahoriz, short dasectnum, char guntype);
extern void operatesprite(short dasprite);
extern void checktouchsprite(short snum, short sectnum);
extern void checkgrabbertouchsprite(short snum, short sectnum);
extern void activatehitag(short dahitag);
extern void processinput(short snum);
extern void movethings(void);
extern void fakedomovethings(void);
extern void fakedomovethingscorrect(void);
extern void doanimations(void);
extern void warp(int32_t *x, int32_t *y, int32_t *z, short *daang, short *dasector);
extern void warpsprite(short spritenum);
extern int testneighborsectors(short sect1, short sect2);
extern void tagcode(void);
extern void bombexplode(int32_t i);
extern void statuslistcode(void);
extern void checkmasterslaveswitch(void);
extern void getpackets(void);
extern void initplayersprite(short snum);
extern void analyzesprites(int32_t dax, int32_t day);
extern void updatesectorz(int32_t x, int32_t y, int32_t z, short *sectnum);
extern void drawoverheadmap(int32_t cposx, int32_t cposy, int32_t czoom, short cang);
extern void drawscreen(short snum, int32_t dasmoothratio);
extern int loadgame(void);
extern int savegame(void);
extern void faketimerhandler(void);
extern void waitforeverybody(void);
#endif

/* cache1d.c */
extern void initcache(intptr_t dacachestart, int32_t dacachesize);
extern void allocache(intptr_t *newhandle, int32_t newbytes, unsigned char *newlockptr);
extern void suckcache(int32_t *suckptr);
extern void agecache(void);
extern void reportandexit(char *errormessage);
extern int32_t initgroupfile(const char *filename);
extern void uninitgroupfile(void);
extern int32_t kopen4load(const char *filename, char searchfirst);
extern int32_t kread(int32_t handle, void *buffer, int32_t leng);
extern int kread8(int32_t handle, char *buffer);
extern int kread16(int32_t handle, short *buffer);
extern int kread32(int32_t handle, int32_t *buffer);
extern int32_t klseek(int32_t handle, int32_t offset, int32_t whence);
extern int32_t kfilelength(int32_t handle);
extern void kclose(int32_t handle);
extern void kdfread(void *buffer, size_t dasizeof, size_t count, int32_t fil);
extern void kdfread8(char *buffer, size_t count, int32_t fil);
extern void kdfread16(short *buffer, size_t count, int32_t fil);
extern void kdfread32(int32_t *buffer, size_t count, int32_t fil);
extern void dfread(void *buffer, size_t dasizeof, size_t count, FILE *fil);
extern void dfread8(char *buffer, size_t count, FILE *fil);
extern void dfread16(short *buffer, size_t count, FILE *fil);
extern void dfread32(int32_t *buffer, size_t count, FILE *fil);
extern void dfwrite(void *buffer, size_t dasizeof, size_t count, FILE *fil);
extern void dfwrite8(char *buffer, size_t count, FILE *fil);
extern void dfwrite16(short *buffer, size_t count, FILE *fil);
extern void dfwrite32(int32_t *buffer, size_t count, FILE *fil);
extern int32_t compress(char *lzwinbuf, int32_t uncompleng, char *lzwoutbuf);
extern int32_t uncompress(char *lzwinbuf, int32_t compleng, char *lzwoutbuf);

/* sdl_driver.c */
extern void _handle_events(void);
extern unsigned char _readlastkeyhit(void);
extern int mprotect_align(const void *addr, size_t len, int prot);
extern void unprotect_ASM_pages(void);
extern void _platform_init(int argc, char **argv, const char *title, const char *icon);
extern int setvesa(int32_t x, int32_t y);
extern int screencapture(char *filename, char inverseit);
extern void setvmode(int mode);
extern int _setgamemode(char davidoption, int32_t daxdim, int32_t daydim);
extern int setgamemode_gl(char davidoption);
extern void setvideodisplaymode(int mode);
extern void setvideodisplaymodenext(int mode);
extern void setvideodrawfit(int on);
extern int getvideodisplaymode(void);
extern void setvideofadeout(void);
extern void setvideofadein(void);
extern int getvideofade(void);
extern int videofading(void);
extern void setvideovsync(int on);
extern int getvideovsync(void);
extern void glredrawlastframe(void);
extern void qsetmode640350(void);
extern void qsetmode640480(void);
extern void getvalidvesamodes(void);
extern int VBE_setPalette(int32_t start, int32_t num, char *palettebuffer);
extern int VBE_getPalette(int32_t start, int32_t num, char *palettebuffer);
extern void _uninitengine(void);
extern void uninitvesa(void);
extern int setupmouse(void);
extern void readmousexy(short *x, short *y);
extern void readmousebstatus(short *bstatus);
extern void _updateScreenRect(int32_t x, int32_t y, int32_t w, int32_t h);
extern void _nextpage(void);
extern unsigned char readpixel(intptr_t offset);
extern void drawpixel(intptr_t offset, unsigned char pixel);
extern void drawpixels(int32_t offset, unsigned short pixels);
extern void drawpixelses(int32_t offset, unsigned int pixelses);
extern void setcolor16(int col);
extern void drawpixel16(int32_t offset);
extern void fillscreen16(int32_t offset, int32_t color, int32_t blocksize);
extern void drawline16(int32_t XStart, int32_t YStart, int32_t XEnd, int32_t YEnd, char Color);
extern void clear2dscreen(void);
extern void _idle(void);
extern void *_getVideoBase(void);
extern void setactivepage(int32_t dapagenum);
extern void limitrate(void);
extern void inittimer(void);
extern void uninittimer(void);
extern void initkeys(void);
extern void uninitkeys(void);
extern void set16color_palette(void);
extern void restore256_palette(void);
extern uint32_t getticks(void);

/* mmulti.c */
extern void callcommit(void);
extern void initcrc(void);
extern int32_t getcrc(char *buffer, short bufleng);
extern void initmultiplayers(char damultioption, char dacomrateoption, char dapriority);
extern void sendpacket(int32_t other, char *bufptr, int32_t messleng);
extern void setpackettimeout(int32_t datimeoutcount, int32_t daresendagaincount);
extern void uninitmultiplayers(void);
extern void sendlogon(void);
extern void sendlogoff(void);
extern int getoutputcirclesize(void);
extern void setsocket(short newsocket);
extern short getpacket(short *other, char *bufptr);
extern void flushpackets(void);
extern void genericmultifunction(int32_t other, char *bufptr, int32_t messleng, int32_t command);

/* engine.c */
extern int setgotpic(int32_t i1);
extern int32_t getclipmask(int i1, int i2, int i3, int i4);
extern int wallfront(int32_t l1, int32_t l2);
#ifdef POLYMOST
extern void polymost_drawrooms(void);
extern void polymost_drawmaskwall(int32_t damaskwallcnt);
extern void polymost_drawsprite(int32_t snum);
extern void polymost_dorotatesprite(int32_t sx, int32_t sy, int32_t z, short a, short picnum, signed char dashade, unsigned char dapalnum, unsigned short dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
extern void polymost_glreset(void);
extern void gltexinvalidate(int32_t dapicnum, int32_t dapalnum, int32_t dameth);
extern void gltexinvalidateall(void);
#endif
#if defined(POLYMOST) && defined(USE_OPENGL)
extern void polymost_fillpolygon(int32_t npoints);
extern void polymost_buildshaderamp(void);
#endif
extern void drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, short daang, int32_t dahoriz, short dacursectnum);
extern int loadboard(char *filename, int32_t *daposx, int32_t *daposy, int32_t *daposz, short *daang, short *dacursectnum);
extern int saveboard(char *filename, int32_t *daposx, int32_t *daposy, int32_t *daposz, short *daang, short *dacursectnum);
extern int setgamemode(char davidoption, int32_t daxdim, int32_t daydim);
extern void setmmxoverlay(int isenabled);
extern int getmmxoverlay(void);
extern void initengine(void);
extern void uninitengine(void);
extern void nextpage(void);
extern void loadtile(short tilenume);
extern intptr_t allocatepermanenttile(short tilenume, int32_t xsiz, int32_t ysiz);
extern int loadpics(char *filename);
extern void qloadkvx(int32_t voxindex, char *filename);
extern int clipinsidebox(int32_t x, int32_t y, short wallnum, int32_t walldist);
extern void drawline256(int32_t x1, int32_t y1, int32_t x2, int32_t y2, unsigned char col);
extern int inside(int32_t x, int32_t y, short sectnum);
extern int getangle(int32_t xvect, int32_t yvect);
extern int ksqrt(int32_t num);
extern void copytilepiece(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz, int32_t tilenume2, int32_t sx2, int32_t sy2);
extern void drawmasks(void);
extern int setsprite(short spritenum, int32_t newx, int32_t newy, int32_t newz);
extern void initspritelists(void);
extern int insertsprite(short sectnum, short statnum);
extern int insertspritesect(short sectnum);
extern int insertspritestat(short statnum);
extern int deletesprite(short spritenum);
extern int deletespritesect(short deleteme);
extern int deletespritestat(short deleteme);
extern int changespritesect(short spritenum, short newsectnum);
extern int changespritestat(short spritenum, short newstatnum);
extern int nextsectorneighborz(short sectnum, int32_t thez, short topbottom, short direction);
extern int cansee(int32_t x1, int32_t y1, int32_t z1, short sect1, int32_t x2, int32_t y2, int32_t z2, short sect2);
extern int lintersect(int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2, int32_t x3, int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz);
extern int rintersect(int32_t x1, int32_t y1, int32_t z1, int32_t vx, int32_t vy, int32_t vz, int32_t x3, int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz);
extern int hitscan(int32_t xs, int32_t ys, int32_t zs, short sectnum, int32_t vx, int32_t vy, int32_t vz, short *hitsect, short *hitwall, short *hitsprite, int32_t *hitx, int32_t *hity, int32_t *hitz, uint32_t cliptype);
extern int neartag(int32_t xs, int32_t ys, int32_t zs, short sectnum, short ange, short *neartagsector, short *neartagwall, short *neartagsprite, int32_t *neartaghitdist, int32_t neartagrange, char tagsearch);
extern void dragpoint(short pointhighlight, int32_t dax, int32_t day);
extern int lastwall(short point);
extern int clipmove(int32_t *x, int32_t *y, int32_t *z, short *sectnum, int32_t xvect, int32_t yvect, int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype);
extern int pushmove(int32_t *x, int32_t *y, int32_t *z, short *sectnum, int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype);
extern void updatesector(int32_t x, int32_t y, short *sectnum);
extern void rotatepoint(int32_t xpivot, int32_t ypivot, int32_t x, int32_t y, short daang, int32_t *x2, int32_t *y2);
extern int initmouse(void);
extern void getmousevalues(short *mousx, short *mousy, short *bstatus);
extern void draw2dgrid(int32_t posxe, int32_t posye, short ange, int32_t zoome, short gride);
extern void draw2dscreen(int32_t posxe, int32_t posye, short ange, int32_t zoome, short gride);
extern void printext256(int32_t xpos, int32_t ypos, short col, short backcol, char name[82], char fontsize);
extern void printext256_noupdate(int32_t xpos, int32_t ypos, short col, short backcol, char name[82], char fontsize);
extern int krand(void);
extern void getzrange(int32_t x, int32_t y, int32_t z, short sectnum, int32_t *ceilz, int32_t *ceilhit, int32_t *florz, int32_t *florhit, int32_t walldist, uint32_t cliptype);
extern void setview(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
extern void calcpixelaspect(void);
extern void setaspect(int32_t daxrange, int32_t daaspect);
extern void flushperms(void);
extern void rotatesprite(int32_t sx, int32_t sy, int32_t z, short a, short picnum, signed char dashade, char dapalnum, unsigned short dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
extern void makepalookup(int32_t palnum, char *remapbuf, signed char r, signed char g, signed char b, char dastat);
extern void setbrightness(char dabrightness, unsigned char *dapal);
extern void setpalettefade(unsigned char r, unsigned char g, unsigned char b, unsigned char offset);
extern void drawmapview(int32_t dax, int32_t day, int32_t zoome, short ang);
extern void clearview(int32_t dacol);
extern void clearallviews(int32_t dacol);
extern void plotpixel(int32_t x, int32_t y, char col);
extern unsigned char getpixel(int32_t x, int32_t y);
extern void setviewtotile(short tilenume, int32_t xsiz, int32_t ysiz);
extern void setviewback(void);
extern void squarerotatetile(short tilenume);
extern void preparemirror(int32_t dax, int32_t day, int32_t daz, short daang, int32_t dahoriz, short dawall, short dasector, int32_t *tposx, int32_t *tposy, short *tang);
extern void completemirror(void);
extern int sectorofwall(short theline);
extern int getceilzofslope(short sectnum, int32_t dax, int32_t day);
extern int getflorzofslope(short sectnum, int32_t dax, int32_t day);
extern void getzsofslope(short sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz);
extern void alignceilslope(short dasect, int32_t x, int32_t y, int32_t z);
extern void alignflorslope(short dasect, int32_t x, int32_t y, int32_t z);
extern int loopnumofsector(short sectnum, short wallnum);
extern void setfirstwall(short sectnum, short newfirstwall);
extern int setrendermode(int renderer);
extern int getrendermode(void);
extern void setrollangle(int32_t rolla);
extern void invalidatetile(short tilenume, int32_t pal, int32_t how);
extern void setpolymost2dview(void);

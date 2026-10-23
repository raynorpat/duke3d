#ifndef _gamever_h_
#define _gamever_h_

/*
 * Game versions, as the dates the gamesrc-ver-recreation project uses for
 *  APPVER_DN3DREV, so its version guards port directly. CMake sets
 *  DUKE3D_GAMEVER from DUKE3D_VERSION; guard game code with, for example,
 *  #if DUKE3D_GAMEVER < GAMEVER_15.
 */
#define GAMEVER_099 199601040L
#define GAMEVER_10  199601290L
#define GAMEVER_11  199602200L
#define GAMEVER_13D 199604190L
#define GAMEVER_14  199610210L
#define GAMEVER_15  199610320L

#ifndef DUKE3D_GAMEVER
#define DUKE3D_GAMEVER GAMEVER_15
#endif

#endif

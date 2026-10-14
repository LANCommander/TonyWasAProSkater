/* GameFMV (GAMEFMV.CPP): the movie table. See build/hdr_notes/career.md (SMovie). */
#ifndef GAME_GAMEFMV_H
#define GAME_GAMEFMV_H

#include "game/types.h"

/* one movie or song (movieDetails[27], 0x800a63e8; demo SMovieDetails, the same layout) */
struct SMovieDetails {
	char *name;             /* 0x00 */
	ushort width;           /* 0x04 */
	ushort height;          /* 0x06 */
	ushort endframe;        /* 0x08 */
	ushort pad0a;           /* 0x0a */
	uint vlcbuffersize;     /* 0x0c */
	uint ringbuffersectors; /* 0x10 */
	int is24bit;            /* 0x14 (a byte used) */
	int startsector;        /* 0x18 0: the file wasn't found */
	int musicScale;         /* 0x1c */
};

/* functions */
struct CVector;
struct Texture;

void GameFMV_SetScreenTexture(uint Checksum);                  /* 0x800229ec */
void GameFMV_AddSoundSource(const CVector *pPos);              /* 0x80022aec */
void GameFMV_PlayMovie(int track);                             /* 0x80022c1c */
void GameFMV_SetStartTrack(int Track);                         /* 0x80023168 */
void GameFMV_StartFrameBufferCopy();                           /* 0x80023260 */
void GameFMV_SkipTrack();                                      /* 0x80023528 */
void GameFMV_SyncFMV(int Arg);                                 /* 0x8002357c */
void GameFMV_RetargetScreenFaces(ushort TPage, Texture *pTex); /* 0x800237b4 */
void strInit();                                                /* 0x80023cb8 */
int GameFMV_ScaleMusicVolume(int Volume);                      /* 0x80023fb0 */
void GameFMV_SetCDMixForPos(const CVector &Pos);               /* 0x80024010 */

#endif

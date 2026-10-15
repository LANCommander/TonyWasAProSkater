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
void FUN_80022b4c();
void GameFMV_PlayMovie(int track);
void GameFMV_SetStartTrack(int a);
void GameFMV_StartFMV(int a);
void GameFMV_StopFMV();
void GameFMV_SyncFMV(int a);
void Redbook_XAAllow(int v);

void GameFMV_SetScreenTexture(uint Checksum);
void GameFMV_AddSoundSource(const CVector *s);
void GameFMV_StartFrameBufferCopy();
void GameFMV_SkipTrack();
void GameFMV_RetargetScreenFaces(ushort arg0, Texture *a1);
void strInit();
int GameFMV_ScaleMusicVolume(int a);
void GameFMV_SetCDMixForPos(const CVector &pos);

/* globals */
extern ushort GameFMV_EndX;
extern int GameFMV_RewindSwitch;
extern short GameFMV_StartY;
#endif

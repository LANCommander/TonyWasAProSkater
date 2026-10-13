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

#endif

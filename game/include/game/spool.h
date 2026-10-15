/* SPOOL.CPP (PSX file / animation spooling). Region/texture data structs live in game/m3d.h; the front
   end's FileLoader (SLoadEntry) in game/shellutils.h. */
#ifndef GAME_SPOOL_H
#define GAME_SPOOL_H

#include "game/types.h"

/* one queued PSX load (SpoolQ[20]) */
struct SQueue {
	char Name[13];      /* 0x00 */
	uchar Occupied;     /* 0x0d */
	uchar FileType;     /* 0x0e */
	uchar Region;       /* 0x0f */
	uchar KeepTextures; /* 0x10: Spool_PSX stores its heap argument here */
	                    /* sizeof == 0x11 */
};

/* an animation packet in the loaded-animation list (Spool_FindAnim) */
struct SAnimFrame;

struct AnimPacket {
	uint *pPacket;     /* 0x00 */
	AnimPacket *pNext; /* 0x04 */
	/* the port's: the packet's frames, anim after anim, as SAnimFrames (retail put the Texture
	   pointers into the file's 8-byte frames) */
	SAnimFrame *pFrames; /* 0x08 */
};

/* functions */
struct Texture;

void Spool_BackupPSX(int region);                                    /* 0x8006ed2c */
void Spool_RestorePSX(int region);                                   /* 0x8006edb8 */
int Spool_GetModel(uint id, int region);                             /* 0x8006ee38 */
Texture *NewTextureEntry(uint checksum);                             /* 0x8006ef30 */
void ProcessNewPSX(int region);                                      /* 0x8006f290 */
void Spool_LoadTexturesToVRAM(int Region, int defrag);               /* 0x8006f854 */
void ClearRegion(int Region, int cleanuptextures, int KeepInMemory); /* 0x8006fde4 */
void Spool_Continue();                                               /* 0x8007046c */
void Spool_Sync();                                                   /* 0x8007079c */

#endif

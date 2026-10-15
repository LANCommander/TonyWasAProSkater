/* SPOOL.CPP (PSX file / animation spooling). Region/texture data structs live in game/m3d.h; the front
   end's FileLoader (SLoadEntry) in game/shellutils.h. */
#ifndef GAME_SPOOL_H
#define GAME_SPOOL_H

#include "game/types.h"
struct Item;

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
void Spool_Init();
void Spool_BackupPSX(int i);
void Spool_RestorePSX(int i);
int Spool_GetModel(uint id, int region);
Item *Spool_FindEnviroItem(uint id);
Texture *Spool_FindTextureEntry(uint id);
void ProcessNewPSX(int region);
int Spool_FindRegion(char *name);
void Spool_ClearPSX(char *name);
void Spool_ClearAllPSXs();
void ClearRegion(int Region, int cleanuptextures, int KeepInMemory);
void Spool_ClearEnvironmentRegions();
SAnimFrame *Spool_FindAnim(char *pName);
int Spool_PSX(char *Name, int IsEnviro, int WhichHeap);
int Spool_GetEnvIndex(int i);
void Spool_Continue();
void Spool_Sync();
int FUN_80070830();

Texture *NewTextureEntry(uint checksum);
void Spool_LoadTexturesToVRAM(int Region, int defrag);

/* globals */
extern Item *EnviroList;
extern int EnvRegions[];
#endif

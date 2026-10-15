/* Line collision (M3DCOLIJ.CPP, M3DZONE.CPP): the line query / result (SLineInfo), the nearest
   hit (SHit), and the item and face caches. Demo layouts; retail agrees (byte-exact TUs). */
#ifndef GAME_COLIJ_H
#define GAME_COLIJ_H

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/types.h"
#include "game/vector.h"
#include "game/m3d.h"

/* a line-collision query and its result (M3dColij_InitLineInfo fills the first part) */
struct SLineInfo {
	Vector StartCoords;          /* 0x00 */
	Vector EndCoords;            /* 0x0c */
	Vector MinCoords;            /* 0x18 */
	Vector MaxCoords;            /* 0x24 */
	int iLo;                     /* 0x30 */
	int iHi;                     /* 0x34 */
	int jLo;                     /* 0x38 */
	int jHi;                     /* 0x3c */
	int Distance;                /* 0x40 */
	int Length;                  /* 0x44 */
	MATRIX WorldCst;             /* 0x48 */
	Item *pItem;                 /* 0x68: the item hit */
	Vector Position;             /* 0x6c: the hit point */
	SVECTOR Normal;              /* 0x78 */
	uint *pFace;                 /* 0x80 */
	int Model;                   /* 0x84 */
	uchar RecordTriggerZoneHits; /* 0x88 */
	uchar DropDown;              /* 0x89 */
	ushort Inquiry;              /* 0x8a */
	int tNear;                   /* 0x8c */
	uint tNumtrLo;               /* 0x90 */
	int tNumtrHi;                /* 0x94 */
	uint tDenomLo;               /* 0x98 */
	int tDenomHi;                /* 0x9c */
	uint NormalOffset;           /* 0xa0 */
	                             /* sizeof == 0xa4 */
};

/* the nearest hit so far (NearestHit). Retail keeps a pointer to the hit face's normal at 0x08
   where the demo had CVector Normal (M3dAsm_LineColijProcessFaces, M3dColij_GetLineInfo,
   M3dColij_LineToThisItemCached store/read it as a pointer). */
struct SHit {
	Item *pItem;        /* 0x00 */
	uint *pFace;        /* 0x04 */
	SVECTOR *pNormal;   /* 0x08 */
	int pad0c[2];       /* 0x0c: the rest of the demo's Normal */
	CVector PolyCentre; /* 0x14 */
	int Dist;           /* 0x20 */
	                    /* sizeof == 0x24 */
};

/* collision item cache (ItemCache[20]) */
struct SColCache {
	Item *pItem;    /* 0x00 */
	SModel *pModel; /* 0x04 */
	int Faces;      /* 0x08 */
	int FirstFace;  /* 0x0c */
};

/* a cached face's bounding box (FaceCache[500]) */
struct SFaceCache {
	int Flags; /* 0x00 */
	int XMin;  /* 0x04 */
	int YMin;  /* 0x08 */
	int ZMin;  /* 0x0c */
	int XMax;  /* 0x10 */
	int YMax;  /* 0x14 */
	int ZMax;  /* 0x18 */
};

/* functions */
void M3dColij_LineToItemZoned(Item **ppItem, SLineInfo *pInfo);

#endif

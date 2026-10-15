/* M3D renderer / level data (M3D*.CPP, SPOOL.CPP, PACK.CPP, PAL.CPP, DB.CPP): the region table
   entry (SPSXRegion), models, faces, textures and palettes, the M3D view of a world item (Item),
   hooks, animation matrices, the display double buffer, viewport, zone grid and a few renderer
   scratch structs.

   Layouts are the THPS2 demo's (MAIN.SYM) where retail agrees; retail differences (SPSXRegion's
   tail, SDoubleBuffer's embedded OT, TSPack's requested size, SFace) come from the byte-exact TUs.
   See build/hdr_notes/m3d.md. */
#ifndef GAME_M3D_H
#define GAME_M3D_H

/* CPPPSX doesn't define __cplusplus, so the SDK headers' own extern "C" guards are inactive under
   egcs: wrap them, as the TUs do */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/types.h"
#include "game/vector.h"
#include "game/dbuffer.h"
class CSuper;

class CGPolyLine;

/* a CLUT in VRAM (PAL.CPP) */
struct tag_S_Pal {
	ushort Clut;          /* 0x00 */
	uchar slot;           /* 0x02 */
	uchar flags;          /* 0x03 */
	ushort Usage;         /* 0x04 */
	uchar InVRAM;         /* 0x06 */
	uchar RemapIndex;     /* 0x07 */
	uint Checksum;        /* 0x08 */
	int m0c;              /* 0x0c */
	tag_S_Pal *pPrevious; /* 0x10: the entry made before (the list runs from the newest, DAT_800d1a80) */
	tag_S_Pal *pNext;     /* 0x14: the one made after */
	                      /* sizeof == 0x18 (NewPaletteEntry) */
};

/* a texture entry (SPOOL.CPP; NewTextureEntry allocates 40 bytes). The first 16 bytes are the
   POLY_FT4 uv/clut/tpage words the renderer copies. */
struct Texture {
	uchar u0;         /* 0x00 */
	uchar v0;         /* 0x01 */
	ushort clut;      /* 0x02 */
	uchar u1;         /* 0x04 */
	uchar v1;         /* 0x05 */
	ushort tpage;     /* 0x06 */
	uchar u2;         /* 0x08 */
	uchar v2;         /* 0x09 */
	uchar u3;         /* 0x0a */
	uchar v3;         /* 0x0b */
	uint TexWin;      /* 0x0c */
	ushort Usage;     /* 0x10 */
	uchar InVRAM : 4; /* 0x12 */
	uchar IsTiling : 4;
	char pad13[0x14 - 0x13];
	uint Checksum;       /* 0x14 */
	tag_S_Pal *pPalette; /* 0x18 */
	ushort x;            /* 0x1c */
	ushort y;            /* 0x1e */
	Texture *pNext;      /* 0x20 */
	Texture *pPrevious;  /* 0x24 */
	                     /* sizeof == 0x28 */
};

/* one frame of a sprite animation (QuickAnimLookup / CBit::mpFrame) */
struct SAnimFrame {
	signed char OffX;  /* 0x00: signed (Bit_Display loads it with lb; egcs char is unsigned) */
	signed char OffY;  /* 0x01 */
	uchar Width;       /* 0x02 */
	uchar Height;      /* 0x03 */
	Texture *pTexture; /* 0x04 */
};

/* a model in a PSX file; the vertices follow (NumVertices of them), then normals and faces */
struct SModel {
	ushort Flags;        /* 0x00 */
	ushort NumVertices;  /* 0x02 */
	ushort NumNormals;   /* 0x04 */
	ushort NumFaces;     /* 0x06 */
	int Radius;          /* 0x08 */
	Vector Box;          /* 0x0c */
	short zMax;          /* 0x18 */
	ushort NextLOD;      /* 0x1a */
	SVECTOR Vertices[1]; /* 0x1c */
};

/* a face in a PSX model (variable size: pTexture only on textured faces). Not in the demo SYM;
   names from the shatter face code (shatter.cpp) */
struct SFace {
	ushort Flags;        /* 0x00 */
	ushort pad2;         /* 0x02 */
	uchar VertexNums[4]; /* 0x04 */
	uint Colour;         /* 0x08 */
	ushort NormalOffset; /* 0x0c */
	ushort Flags2;       /* 0x0e: terrain/collision flags (the physics TUs' "Flags") */
	uint TexWord;        /* 0x10: textured (Flags & 1): the index in the region's texture table
	                        (SPSXRegion::ppTextures); from M3dInit_ParsePSX on, with Flags & 2, the
	                        texture itself (its uv/clut word; a Dreamcast face's PC_DC_FACE_TEX) */
};

/* a hook point on a super item's part (SPSXRegion::pHooks) */
struct SHook {
	SVector Offset; /* 0x00 */
	ushort Part;    /* 0x06 */
};

/* a skater's hooks packet as M3dUtils_ReadHooksPacket takes it (opaque here) */
struct SHooksPacket {
	char Data[20]; /* 0x00 */
};

/* a decompressed animation frame entry: 3x3 rotation and translation, all shorts */
struct SMatrix {
	short m[3][3]; /* 0x00 */
	short t[3];    /* 0x12 */
};

/* compressed-animation pivot (DECOMP.CPP) */
struct SEulerPivot {
	short vx; /* 0x00 */
	short vy; /* 0x02 */
	short vz; /* 0x04 */
	short px; /* 0x06 */
	short py; /* 0x08 */
	short pz; /* 0x0a */
};

/* hierarchy data (CSuper::mpJoints / mpLinks; demo layouts, not touched by any TU yet) */
struct SJoint {
	SVector Angles;       /* 0x00 */
	SVector Displacement; /* 0x06 */
};

struct SLink {
	ushort Part;       /* 0x00 */
	ushort ParentPart; /* 0x02 */
	SVector Pivot;     /* 0x04 */
	ushort ParentLink; /* 0x0a */
};

/* an item's lighting (CItem::mpLight; demo layout, not touched by any TU yet) */
struct SLight {
	short LightMatrix[3][3]; /* 0x00 */
	ushort Pad1;             /* 0x12 */
	short ColorMatrix[3][3]; /* 0x14 */
	ushort Pad2;             /* 0x26 */
	int BackColor[3];        /* 0x28 */
	                         /* sizeof == 0x34 */
};

/* a loaded PSX file / level region (PSXRegion[]) */
struct SPSXRegion {
	char Filename[9];        /* 0x00 */
	uchar IsSuper;           /* 0x09 */
	uchar Usable;            /* 0x0a */
	uchar Protected;         /* 0x0b */
	uint *pModelChecksums;   /* 0x0c */
	SModel **ppModels;       /* 0x10: the port's own array of the file's models (ProcessNewPSX) */
	uint *pPSX;              /* 0x14 */
	uint *pAnimFile;         /* 0x18 */
	ushort *pHierarchy;      /* 0x1c */
	SHook *pHooks;           /* 0x20 */
	uint *pColourTable;      /* 0x24 */
	uint *pTexWibData;       /* 0x28 */
	uint *pColourPulseData;  /* 0x2c */
	ushort NumParts;         /* 0x30: retail drops the demo's mpAnimTranslation */
	uchar Fastable;          /* 0x32 */
	uchar TexturesInRAM;     /* 0x33 */
	uchar KeepTexturesInRAM; /* 0x34 */
	char pad35[0x38 - 0x35];
	/* the port's: the textures of the file's texture table (its faces name them by index until
	   M3dInit_ParsePSX), and the counts; retail patched pointers into the file's 32-bit slots */
	Texture **ppTextures; /* 0x38 */
	ushort NumModels;     /* 0x3c */
	ushort NumTextures;   /* 0x3e */
	/* sizeof == 0x40 */
};

/* M3D's plain view of a world item: CItem's data (game/ob.h) without the class. M3D/SPOOL code
   walks item lists through it; CBody/CSuper fields are reached through those classes. A PSX file's
   environment items (36-byte records after its header) become Items of their own in the port:
   PSXRegionItems. */
struct Item {
	ushort Flags;    /* 0x00 */
	ushort Inquiry;  /* 0x02 */
	Vector Position; /* 0x04 */
	SVector Angles;  /* 0x10 */
	ushort Model;    /* 0x16 */
	uchar Frame;     /* 0x18 */
	uchar TintIndex; /* 0x19 */
	uchar Anim;      /* 0x1a */
	uchar Region;    /* 0x1b */
	Item *NextItem;  /* 0x1c */
	uint RGB;        /* 0x20 */
	SVector Scale;   /* 0x24 */
	/* the port's (level items only): the hide flags (1, 0x40) a script's SETVISIBILITYINBOX set,
	   which drawing the entire level ignores (M3d_Render), and the ones it lifted for a frame */
	uchar BoxHidden; /* 0x2a */
	uchar BoxLifted; /* 0x2b */
	int SemiTrans;            /* 0x2c: CItem::mTRN */
	Item *PreviousItem;       /* 0x30 */
	uint StoredRGB;           /* 0x34 */
	uchar PredatorEffectType; /* 0x38 */
	uchar PredatorEffectFade; /* 0x39 */
	ushort Type;              /* 0x3a */
	SLight *pLight;           /* 0x3c */
	ushort FlashTimer;        /* 0x40 */
	short Flash_dR;           /* 0x42 */
	short Flash_dG;           /* 0x44 */
	short Flash_dB;           /* 0x46 */
	                          /* 0x48: CItem's vptr */
};

/* the port's: each region's items (its PSX file's records) as Items, linked through NextItem in
   file order (M3dInit_ParsePSX; retail used the file's records themselves) */
extern Item *PSXRegionItems[40];
/* the Item of the record at byte offset `offset` of region r's PSX file (12 + 36 * index), as the
   file's other packets refer to them */
#define PSX_ITEM(r, offset) (PSXRegionItems[r] + ((offset) - 12) / 36)

/* the camera's view rectangle and clip planes (pCurrentViewport) */
struct SViewport {
	ushort xR;      /* 0x00 */
	ushort yB;      /* 0x02 */
	ushort xL;      /* 0x04 */
	ushort yT;      /* 0x06 */
	ushort Hither;  /* 0x08 */
	ushort Yon;     /* 0x0a */
	ushort TanHalf; /* 0x0c */
	ushort Zoom;    /* 0x0e */
	ushort xC;      /* 0x10 */
	ushort yC;      /* 0x12 */
};

/* one display buffer (DoubleBuffer[2], pDoubleBuffer): game/dbuffer.h */

/* a pulsing colour entry (M3d_PreprocessPulsingColours) */
struct SRGBI {
	uchar r;        /* 0x00 */
	uchar g;        /* 0x01 */
	uchar b;        /* 0x02 */
	uchar Interval; /* 0x03 */
};

/* texture wibble data (SPSXRegion::pTexWibData) */
struct STexWibVertInfo {
	uchar u;         /* 0x00 */
	uchar v;         /* 0x01 */
	uchar uAmpPhase; /* 0x02 */
	uchar vAmpPhase; /* 0x03 */
};

struct STexWibFaceInfo {
	STexWibVertInfo p0; /* 0x00 */
	STexWibVertInfo p1; /* 0x04 */
	STexWibVertInfo p2; /* 0x08 */
	STexWibVertInfo p3; /* 0x0c */
};

/* the zone grid over the level's items (Zones[]) */
struct SZone {
	int Flags;         /* 0x000 */
	int xMin;          /* 0x004 */
	int zMin;          /* 0x008 */
	int xMax;          /* 0x00c */
	int zMax;          /* 0x010 */
	int ZoneWidth;     /* 0x014 */
	int ZoneHeight;    /* 0x018 */
	short Width;       /* 0x01c */
	short Height;      /* 0x01e */
	Item *Ptr[20][20]; /* 0x020 */
	                   /* sizeof == 0x660 */
};

/* a VRAM allocation block (PACK.CPP) */
struct TSPack {
	ushort X;        /* 0x00 */
	ushort Y;        /* 0x02 */
	ushort W;        /* 0x04 */
	ushort H;        /* 0x06 */
	TSPack *pNext;   /* 0x08 */
	char Type;       /* 0x0c */
	uchar no_bottom; /* 0x0d */
	uchar bpp;       /* 0x0e */
	uchar aligned;   /* 0x0f */
	short ReqW;      /* 0x10: THPS1 records the requested size */
	short ReqH;      /* 0x12 */
	                 /* sizeof == 0x14 */
};

/* ribbon/polyline screen-space scratch (BIT.CPP) */
struct SCalcBuffer {
	int Pos;       /* 0x00: packed screen XY */
	int NormalX;   /* 0x04 */
	int NormalY;   /* 0x08 */
	int Zed;       /* 0x0c */
	int ScrWidth;  /* 0x10 */
	int ScrWidthB; /* 0x14 */
	int Bad;       /* 0x18 */
};

/* a line clipped for drawing (CBit display) */
struct SClipLineBuf {
	CVector Start; /* 0x00 */
	CVector End;   /* 0x0c */
	uint ScrStart; /* 0x18 */
	uint ScrEnd;   /* 0x1c */
};

/* one segment of a CPolyLine/CGPolyLine */
struct SLineSeg {
	CVector End; /* 0x00 */
	uchar r;     /* 0x0c */
	uchar g;     /* 0x0d */
	uchar b;     /* 0x0e */
	uchar code;  /* 0x0f */
};

/* a rain streak (CScriptOnlyBaddy rain) */
struct SRainEntry {
	CVector vel;        /* 0x00 */
	CGPolyLine *p_line; /* 0x0c */
};

/* functions */
struct SCamera;
struct SLineInfo;
void M3d_RenderBackground(void *p);
void M3d_BuildTransform(CSuper *t);

void M3d_RenderSetup(SCamera *pCamera, SViewport *pVP, P_TAG *ot);
void M3d_RenderCleanup();
void M3d_Render(void *p);
void RenderModel(SModel *p);
void RenderModelFast(SModel *p, VECTOR *c);
void RenderModelInSuper(SModel *p);
void RenderModelNonRotated(SModel *p, VECTOR *c);

/* globals */
extern uchar CurrentObjFileRegion;
extern int CurrentWaterLevel;
extern int DpqMax;
extern int DpqMin;
extern int DpqShift;
extern uint M3d_FadeColour;
extern short M3d_OTPushback2;
extern int M3d_WireframeMode;
extern SCamera *pCurrentCamera;
extern int PixelAspectX;
extern int PixelAspectY;
extern uint Xres;
extern uint Yres;
#endif

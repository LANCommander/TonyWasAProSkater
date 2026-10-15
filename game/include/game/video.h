/* Replay recording/playback (VIDEO.CPP): the per-replay stream state (GVInfo1/GVInfo2), the
   replay header saved to the memory card, and the restart settings (GVideoRestart). Plain data,
   no member functions. Retail layouts are the demo's (MAIN.SYM). See build/hdr_notes/video.md. */
#ifndef GAME_VIDEO_H
#define GAME_VIDEO_H

/* CPPPSX doesn't define __cplusplus, so the SDK headers' own extern "C" guards are inactive under
   egcs: wrap them, as the TUs do */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/types.h"
#include "game/vector.h"
#include "game/classsizes.h"

/* Video_Start */
enum EVideoMode { VIDEO_OFF, VIDEO_RECORD, VIDEO_PLAYBACK };

/* a replay highlight (frame range and its score) */
struct SHighlight {
	short FromFrame; /* 0x0 */
	short ToFrame;   /* 0x2 */
	int Score;       /* 0x4 */
}; /* size 0x8 */

/* one replay's stream and camera state (GVInfo1 0x800bf318, GVInfo2 0x800bf3d8) */
struct SVideoInfo {
	int VideoMode;            /* 0x00 EVideoMode (demo type; int as 13 of 14 TUs have it) */
	ulong *pBlockStart;       /* 0x04 */
	ulong *pStreamStart;      /* 0x08 */
	ulong *pStreamEnd;        /* 0x0c */
	ulong *pStream;           /* 0x10 */
	ulong Bits;               /* 0x14 */
	int BitsLeft;             /* 0x18 */
	ulong NumFrames;          /* 0x1c */
	ulong FramesLeft;         /* 0x20 */
	int CurCameraIndex;       /* 0x24 */
	CVector CamPos;           /* 0x28 */
	int CamMode;              /* 0x34 */
	int CamXblanks;           /* 0x38 */
	int TabuCam;              /* 0x3c */
	int NoLOS;                /* 0x40 */
	int OldCameraIndex;       /* 0x44 */
	int TrailingCameraType;   /* 0x48 */
	int ReplayHighlight;      /* 0x4c */
	SHighlight Highlights[5]; /* 0x50 */
	int OldPhysicsState;      /* 0x78 demo: EPhysicsState */
	int OldAnim;              /* 0x7c */
	int OldFrame;             /* 0x80 */
	SVECTOR Euler;            /* 0x84 */
	SVECTOR OldEuler;         /* 0x8c */
	CVector OldPos;           /* 0x94 */
	CVector OldPos2;          /* 0xa0 */
	int OldFlipAngle;         /* 0xac */
	int FlipAngleDelta;       /* 0xb0 */
	int OldXblanks;           /* 0xb4 */
	int Yblanks;              /* 0xb8 */
	int PlaybackFrozen;       /* 0xbc */
}; /* size 0xc0 */
CHECK_CLASS_SIZE(SVideoInfo, SIZEOF_SVIDEOINFO);

/* the replay header (at pBlockStart + 128 words; Video_SetHeader) */
struct SVideoHead {
	ulong NumFrames;          /* 0x00 */
	SHighlight Highlights[5]; /* 0x04 */
	short NumSkaters;         /* 0x2c */
	short Skater;             /* 0x2e */
	short Skater2;            /* 0x30 */
	short Level;              /* 0x32 */
	short GGame;              /* 0x34 */
	short Checksum;           /* 0x36 */
	char VideoFlags;          /* 0x38 */
	char Padding[7];          /* 0x39 */
}; /* size 0x40 */

/* what to restart after a replay (GVideoRestart) */
struct SVideoRestart {
	int NumSkaters; /* 0x00 */
	int Skater;     /* 0x04 */
	int Skater2;    /* 0x08 */
	int Level;      /* 0x0c */
	int GGame;      /* 0x10 */
}; /* size 0x14 */

/* functions */
class CBruce;
void Video_SFXPlay(int a, short b, int c);
void Video_BloodSplat(CBruce *b, int a, int c);
void FUN_80068f40(int a, int b);
void Video_HighlightStart(CBruce *b);
void Video_HighlightEnd(CBruce *pSkater, int Score);
void Video_SortAndMerge();
void Video_SetHeader(SVideoHead *h, int which);
void Video_InitOnce();
void FUN_8006952c();
void Video_Stop(CBruce *b);
void Video_Record(CBruce *b);
void Video_Playback(CBruce *pSkater);

int HasSoundCmd();
void Video_SyncObsIfNeeded();
void Video_Cleanup();
void Video_Start(CBruce *a, EVideoMode b, int c, int d);
void GetDataChunk(CBruce *pSkater, int NChunks);
void Video_HandleStepping();

/* globals */
extern int GVideoReplay;
extern SVideoInfo GVInfo1[];
#endif

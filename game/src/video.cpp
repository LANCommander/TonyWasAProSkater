/* VIDEO.CPP: replay recording and playback: the bit-packed per-frame skater stream (position,
   angles, animation, sounds, blood splats, trick objects), the top-5 highlights, the replay header
   and pad stepping through a frozen replay (declarations in game/video.h). */
#include "game/video.h"
#include "game/bruce.h"
#include "game/mem.h"
#include "game/classsizes.h"
#include <sys/types.h>
#include <libgte.h>
#include "game/trickob.h"
#include "game/m3d.h"
#include "game/colij.h"
#include "game/career.h"
#include "game/psyq.h"
#include "pc_frame.h"
#include "game/bit.h"
#include "game/camera.h"
#include "game/front.h"
#include "game/platform.h"
#include "game/pad.h"
#include "game/bit2.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/main.h"
#include "game/physics.h"
#include "game/sfx.h"
#include "game/spool.h"
#include "game/trig.h"
#include "game/campt.h"

/* a queued replay sound command (SoundCmd[30]) */
typedef struct {
	int type, val;
} SSoundCmd;
enum ESoundCmdType { SOUNDCMD_NONE, SOUNDCMD_PLAY, SOUNDCMD_STOP };

extern SSoundCmd SoundCmd[];
extern int FirstEntry, LastEntry;
extern int Splat1, Splat2, TrickOb1, TrickOb2;
extern int HLStartCalled, HighlightStartFrame, HighlightEndFrame, HighlightStartY, HighlightEndY;
extern int ReplaySoundLockout;
extern int PosBits[], EulerBits[], FlipAngleBits[], PrefixCode[], PrefixBits[];
extern int DAT_800d1df0;

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

extern "C" void FUN_80068d88();
extern "C" void FUN_80068ed4();
extern "C" void FUN_80068f2c();
extern "C" int Mips_DotProd2(int, int, int, int);
void FUN_80095cb0(int, MATRIX *);
int BitsRequired(int);
void PutNBits(SVideoInfo *, int, int);
void FlushBits(SVideoInfo *);
int GetBit(SVideoInfo *);
int GetNBitsSigned(SVideoInfo *, int);
u_long GetNBitsUnsigned(SVideoInfo *, int);
void QueueSoundCmd(ESoundCmdType, int);
void DequeueSoundCmd(ESoundCmdType &, int &);
void Video_SyncObs(int);
void Matrix2Euler(MATRIX *, SVECTOR *);
void PutDataChunk(CBruce *);
void M3dZone_LineToItem(SLineInfo *, int);

/* BitsRequired__Fl (0x80068ae0) */
int BitsRequired(int x) {
	int n = 2;
	int m;

	if (x < 0) {
		for (m = -2; x < m; m <<= 1)
			n++;
	} else {
		for (m = 2; x >= m; m <<= 1)
			n++;
	}

	return n;
}

/* PutNBits__FP10SVideoInfoli (0x80068b34) */
void PutNBits(SVideoInfo *v, int val, int n) {
	if (n == 0)
		return;

	if (v->BitsLeft < n) {
		n -= v->BitsLeft;
		v->Bits = (v->Bits & ((1 << (32 - v->BitsLeft)) - 1)) | (val << (32 - v->BitsLeft));
		val >>= v->BitsLeft;
		*v->pStream++ = v->Bits;
		v->Bits = 0;
		v->BitsLeft = 32;
	}

	{
		int a = v->BitsLeft;

		v->BitsLeft = a - n;
		v->Bits = (v->Bits & ((1 << (32 - a)) - 1)) | (val << (32 - a));
	}

	if (v->BitsLeft == 0) {
		*v->pStream++ = v->Bits;
		v->Bits = 0;
		v->BitsLeft = 32;
	}
}

/* FlushBits__FP10SVideoInfo (0x80068bf8) */
void FlushBits(SVideoInfo *v) {
	if (v->BitsLeft != 32) {
		*v->pStream++ = v->Bits;
		v->Bits = 0;
		v->BitsLeft = 32;
	}
}

/* GetNBitsUnsigned__FP10SVideoInfoi (0x80068c30) */
u_long GetNBitsUnsigned(SVideoInfo *v, int n) {
	unsigned r;
	int a;

	if (v->BitsLeft == 0) {
		v->Bits = *v->pStream++;
		v->BitsLeft = 32;
	}

	a = v->BitsLeft;

	if (a >= n) {
		v->BitsLeft = a - n;
		r = v->Bits & ((1 << n) - 1);
		v->Bits >>= n;
	} else {
		n -= a;
		r = v->Bits & ((1 << a) - 1);
		v->Bits = *v->pStream++;
		v->BitsLeft = 32 - n;
		r |= (v->Bits & ((1 << n) - 1)) << a;
		v->Bits >>= n;
	}

	return r;
}

/* GetNBitsSigned__FP10SVideoInfoi (0x80068cec) */
int GetNBitsSigned(SVideoInfo *v, int n) {
	unsigned x = GetNBitsUnsigned(v, n);
	unsigned m = 1 << (n - 1);

	if (x & m)
		x |= -(1 << n);

	return x;
}

/* GetBit__FP10SVideoInfo (0x80068d38) */
int GetBit(SVideoInfo *v) {
	int b;

	if (v->BitsLeft == 0) {
		v->Bits = *v->pStream++;
		v->BitsLeft = 32;
	}

	v->BitsLeft--;
	b = v->Bits & 1;
	v->Bits >>= 1;
	return b;
}

/* FUN_80068d88 (0x80068d88) */
void FUN_80068d88(void) {
	FirstEntry = 0;
	LastEntry = 0;
}

/* QueueSoundCmd__F13ESoundCmdTypei (0x80068d98) */
void QueueSoundCmd(ESoundCmdType type, int val) {
	SoundCmd[LastEntry].type = type;
	SoundCmd[LastEntry].val = val;
	LastEntry = (LastEntry + 1) % 30;
}

/* HasSoundCmd__Fv (0x80068df8) */
int HasSoundCmd(void) {
	return FirstEntry != LastEntry;
}

/* DequeueSoundCmd__FR13ESoundCmdTypeRi (0x80068e10) */
void DequeueSoundCmd(ESoundCmdType &type, int &val) {
	type = (ESoundCmdType)SoundCmd[FirstEntry].type;
	val = SoundCmd[FirstEntry].val;
	FirstEntry = (FirstEntry + 1) % 30;
}

/* Video_SFXPlay__Fisi (0x80068e8c) */
/* */
void Video_SFXPlay(int a, short b, int c) {
	if (GVInfo1[0].VideoMode == VIDEO_RECORD && GamePaused == 0)
		QueueSoundCmd(SOUNDCMD_PLAY, a);
}

/* FUN_80068ed4 (0x80068ed4) */
void FUN_80068ed4(void) {
	Splat2 = -1;
	Splat1 = -1;
}

/* Video_BloodSplat__FP6CBruceii (0x80068ee8) */
/* */
void Video_BloodSplat(CBruce *b, int a, int c) {
	if (GVInfo1[0].VideoMode == VIDEO_RECORD) {
		if (b == MechList)
			Splat1 = a;
		else
			Splat2 = a;
	}
}

/* FUN_80068f2c (0x80068f2c) */
void FUN_80068f2c(void) {
	TrickOb2 = -1;
	TrickOb1 = -1;
}

/* FUN_80068f40 (0x80068f40) */
void FUN_80068f40(int a, int b) {
	if (GVInfo1[0].VideoMode == VIDEO_RECORD) {
		switch (a) {
			case 1:
				TrickOb1 = b;
				break;
			case 2:
				TrickOb2 = b;
				break;
		}
	}
}

/* Video_HighlightStart__FP6CBruce (0x80068f84) */
/* */
void Video_HighlightStart(CBruce *b) {
	SVideoInfo *v;

	if (b == MechList)
		v = &GVInfo1[0];
	else
		v = &GVInfo1[1];

	if (!HLStartCalled) {
		int t = v->NumFrames, x = b->mPos.vy;

		HLStartCalled = 1;
		HighlightStartFrame = t;
		HighlightStartY = x;
	}
}

/* Video_HighlightEnd__FP6CBrucei (0x80068fd8) */
/* Demo twin. Inserts a finished trick into the player's top-5 replay highlights. */
void Video_HighlightEnd(CBruce *pSkater, int Score) {
	SVideoInfo *VInfo;
	int WasABail;

	if (pSkater == MechList)
		VInfo = &GVInfo1[0];
	else
		VInfo = &GVInfo1[1];

	HLStartCalled = 0;
	HighlightEndFrame = VInfo->NumFrames;
	HighlightEndY = pSkater->mPos.vy;
	WasABail = (Score < 0);

	if (Score < 0)
		Score = -Score;

	if (Score == 0)
		Score = (ABS(HighlightEndY - HighlightStartY) * 2) >> 12;
	else
		Score += ABS(HighlightEndY - HighlightStartY) >> 13;

	for (int i = 0; i < 5; i++) {
		if (VInfo->Highlights[i].Score < Score) {
			for (int j = 4; j > i; j--)
				VInfo->Highlights[j] = VInfo->Highlights[j - 1];

			HighlightStartFrame -= 15;

			if (HighlightStartFrame < 0)
				HighlightStartFrame = 0;

			if (WasABail)
				HighlightEndFrame += 30;
			else
				HighlightEndFrame += 8;

			if (HighlightEndFrame >= 3600)
				HighlightEndFrame = 3599;

			VInfo->Highlights[i].FromFrame = HighlightStartFrame;
			VInfo->Highlights[i].ToFrame = HighlightEndFrame;
			VInfo->Highlights[i].Score = Score;
			break;
		}
	}
}

/* Video_SortAndMerge__Fv (0x80069164) */
void Video_SortAndMerge(void) {
	SHighlight TempHL[5];
	SHighlight NullHL;

	NullHL.FromFrame = NullHL.ToFrame = NullHL.Score = 0;

	{
		int i;

		for (i = 0; i < 5; i++) {
			TempHL[i] = NullHL;

			{
				int BestFrom = 0x7fffffff;
				int BestIndex = -1;
				int j;

				for (j = 0; j < 5; j++) {
					if (GVInfo1[0].Highlights[j].ToFrame >= GVInfo1[0].NumFrames)
						GVInfo1[0].Highlights[j].ToFrame = GVInfo1[0].NumFrames - 1;

					if (GVInfo1[0].Highlights[j].Score &&
					    GVInfo1[0].Highlights[j].FromFrame < BestFrom) {
						BestFrom = GVInfo1[0].Highlights[j].FromFrame;
						BestIndex = j;
					}
				}

				if (BestIndex != -1) {
					TempHL[i] = GVInfo1[0].Highlights[BestIndex];
					GVInfo1[0].Highlights[BestIndex].Score = 0;
				}
			}
		}
	}

	{
		int i;

		for (i = 4; i > 0; i--) {
			if (TempHL[i].FromFrame <= TempHL[i - 1].ToFrame + 15 &&
			    TempHL[i].FromFrame > TempHL[i - 1].FromFrame) {
				if (TempHL[i - 1].ToFrame < TempHL[i].ToFrame)
					TempHL[i - 1].ToFrame = TempHL[i].ToFrame;

				TempHL[i - 1].Score += TempHL[i].Score;
				TempHL[i] = NullHL;
			}
		}
	}

	{
		int i;
		int n = 0;

		for (i = 0; i < 5; i++) {
			GVInfo1[0].Highlights[i] = NullHL;

			if (TempHL[i].Score) {
				GVInfo1[0].Highlights[n] = TempHL[i];
				n++;
			}
		}
	}
}

/* Video_SetHeader__FP10SVideoHeadi (0x80069394) */
/* */
void Video_SetHeader(SVideoHead *h, int which) {
	SVideoInfo v;
	int i;
	CBruce *b, *o;

	{
		SVideoInfo *p;

		if (which)
			p = &GVInfo1[1];
		else
			p = &GVInfo1[0];

		v = *p;
	}

	h->NumFrames = v.NumFrames;

	for (i = 0; i < 5; i++)
		h->Highlights[i] = v.Highlights[i];

	{
		unsigned short t = *(unsigned short *)&GNumberOfPlayers; /* (a halfword read) */

		b = MechList;
		h->NumSkaters = t;
	}

	/* the other skater is linked through the item list */
	o = (CBruce *)b->mNextItem;

	switch (which) {
		case 0:
			h->Skater = b->mSkaterIndex;

			if (o)
				h->Skater2 = o->mSkaterIndex;

			break;
		case 1:
			h->Skater = o->mSkaterIndex;

			if (o)
				h->Skater2 = b->mSkaterIndex;

			break;
	}

	{
		unsigned short x = *(unsigned short *)&GLevel, y = *(unsigned short *)&GGame;

		h->Checksum = 0;
		h->Level = x;
		h->GGame = y;
	}
}

/* Video_InitOnce__Fv (0x800694a0) */
/* Demo twin (THPS1 allocates bigger replay buffers and skips a header); locals from
   its MAIN.SYM. Allocates player 1's replay buffer, and player 2's in a two-player game. */
void Video_InitOnce() {
	GVInfo1[0].pBlockStart = GVInfo1[0].pStreamStart = (ulong *)Mem_NewX(24576, 1, 1);
	GVInfo1[0].pStreamStart += 576 / 4;

	if (GNumberOfPlayers == 2) {
		GVInfo1[1].pBlockStart = GVInfo1[1].pStreamStart = (ulong *)Mem_NewX(24576, 1, 1);
		GVInfo1[1].pStreamStart += 576 / 4;
	} else {
		GVInfo1[1].pBlockStart = GVInfo1[1].pStreamStart = 0;
	}
}

/* FUN_8006952c (0x8006952c) */
void FUN_8006952c(void) {}

/* Video_SyncObs__Fi (0x80069534) */
/* */
void Video_SyncObs(int count) {
	CBruce *b = MechList;
	CBruce *o;
	int i;

	ReplaySoundLockout = 1;
	o = (CBruce *)b->mNextItem;

	for (i = 0; i < count; i++) {
		Ob_ClearInputFlags(BaddyList);
		Ob_ClearInputFlags(EnvironmentalObjectList);
		GetDataChunk(b, -1);

		if (o)
			GetDataChunk(o, -1);

		Ob_QuickAI(&EnvironmentalObjectList);
		Ob_AI(&BaddyList);
	}

	ReplaySoundLockout = 0;
}

/* Video_SyncObsIfNeeded__Fv (0x800695f4) */
/* */
void Video_SyncObsIfNeeded() {
	int n = GVideoReplay;

	if (n == 1) {
		if (GVInfo1[0].Highlights[0].FromFrame == 1)
			return;

		Video_SyncObs(GVInfo1[0].Highlights[0].FromFrame - 1);
	} else {
		Video_SyncObs(GVInfo1[0].Highlights[n - 1].FromFrame - GVInfo1[0].Highlights[n - 2].ToFrame - 2);
	}
}

/* Video_Cleanup__Fv (0x80069670) */
/* SVideoInfo[2] */
void Video_Cleanup(void) {
	GVInfo1[0].VideoMode = VIDEO_OFF;
	GVInfo1[1].VideoMode = VIDEO_OFF;
}

/* Video_Start__FP6CBruce10EVideoModeii (0x80069684) */
void Video_Start(CBruce *a, EVideoMode b, int c, int d) {
	SVideoInfo *s;
	int i, j, y;
	ulong *x;
	ulong *q;
	int g;

	if (a == MechList)
		s = GVInfo1;
	else
		s = (&GVInfo1[1]);

	HLStartCalled = 0;
	FUN_80068d88();
	FUN_80068ed4();
	FUN_80068f2c();

	s->VideoMode = b;

	if (b == 1) {
		s->NumFrames = 0;
		s->BitsLeft = 32;

		for (j = 0; j < 5; j++)
			s->Highlights[j].Score = 0;

		q = s->pBlockStart;
		s->ReplayHighlight = 0;

		for (i = 0; i < 6144; i++) {
			*q = 0;
			q++;
		}
	} else {
		s->BitsLeft = 0;
	}

	x = s->pStreamStart;
	y = s->NumFrames;
	s->Bits = 0;
	s->CurCameraIndex = -1;
	s->OldCameraIndex = -2;
	s->CamXblanks = 0;
	s->NoLOS = 0;
	s->TabuCam = -1;
	s->pStream = x;
	s->FramesLeft = y;

	if (d != 0x7fffffff)
		s->FramesLeft = d - c + 1;

	g = Xblanks;
	s->OldPos.vz = 0;
	s->OldPos.vy = 0;
	s->OldPos.vx = 0;
	s->OldPos2.vz = 0;
	s->OldPos2.vy = 0;
	s->OldPos2.vx = 0;
	s->Euler.vz = 0;
	s->Euler.vy = 0;
	s->Euler.vx = 0;
	s->OldEuler.vz = 0;
	s->OldEuler.vy = 0;
	s->OldEuler.vx = 0;
	s->OldPhysicsState = 0;
	s->OldAnim = 0;
	s->OldFrame = 0;
	s->OldFlipAngle = 0;
	s->PlaybackFrozen = 0;
	s->OldXblanks = g;
	s->Yblanks = g;
	a->mAnim = 0;
	a->mFrame = 0;
	a->m724 = 0;
	a->BoardOn();
}

/* Video_Stop__FP6CBruce (0x8006981c) */
/* */
void Video_Stop(CBruce *b) {
	SVideoInfo *v;

	if (b == MechList)
		v = &GVInfo1[0];
	else
		v = &GVInfo1[1];

	if (v->VideoMode == VIDEO_RECORD)
		FlushBits(v);

	v->VideoMode = VIDEO_OFF;
	GVideoReplay = 0;
}

/* Matrix2Euler__FP6MATRIXP7SVECTOR (0x80069878) */
void Matrix2Euler(MATRIX *mat, SVECTOR *out) {
	MATRIX m = *mat;
	int x, y, z;

	m.m[0][1] = -m.m[0][1];
	m.m[1][1] = -m.m[1][1];
	m.m[2][1] = -m.m[2][1];
	y = ratan2(m.m[0][2], m.m[2][2]) & 4095;
	FUN_80095cb0(-y, &m);

	x = ratan2(-m.m[1][2], m.m[2][2]) & 4095;
	FUN_80095cb0(-x, &m);

	z = ratan2(m.m[1][0], m.m[1][1]) & 4095;
	out->vy = y;
	out->vx = x;
	out->vz = z;
}

/* PutDataChunk__FP6CBruce (0x80069974) */
/* Locals, their order and types come from the demo's MAIN.SYM. */
void PutDataChunk(CBruce *pSkater) {
	int i, bx, by, bz;
	int PredX, PredY, PredZ;
	int DeltaX, DeltaY, DeltaZ;
	CVector PredPos, DeltaPos;
	SVideoInfo *VInfo;

	if (pSkater == MechList)
		VInfo = &GVInfo1[0];
	else
		VInfo = &GVInfo1[1];

	if (VInfo->pStream + 6 >= VInfo->pStreamStart + 6000) {
		Video_Stop(pSkater);
		return;
	}

	VInfo->NumFrames++;

	PredX = VInfo->OldPos.vx * 2 - VInfo->OldPos2.vx;
	PredY = VInfo->OldPos.vy * 2 - VInfo->OldPos2.vy;
	PredZ = VInfo->OldPos.vz * 2 - VInfo->OldPos2.vz;
	DeltaX = (pSkater->mPos.vx >> 12) - PredX;
	DeltaY = (pSkater->mPos.vy >> 12) - PredY;
	DeltaZ = (pSkater->mPos.vz >> 12) - PredZ;
	VInfo->OldPos2 = VInfo->OldPos;
	VInfo->OldPos = pSkater->mPos >> 12;
	bx = BitsRequired(DeltaX);
	by = BitsRequired(DeltaY);
	bz = BitsRequired(DeltaZ);

	if (bx < by)
		bx = by;

	if (bx < bz)
		bx = bz;

	for (i = 0; i < 6; i++)
		if (PosBits[i] >= bx)
			break;

	PutNBits(VInfo, PrefixCode[i], PrefixBits[i]);
	PutNBits(VInfo, DeltaX, PosBits[i]);
	PutNBits(VInfo, DeltaY, PosBits[i]);
	PutNBits(VInfo, DeltaZ, PosBits[i]);

	Matrix2Euler(&pSkater->mRotMatrix, &VInfo->Euler);

	DeltaX = ((VInfo->Euler.vx & 0xfff) - VInfo->OldEuler.vx + 0x4000) & 0xfff;
	DeltaY = ((VInfo->Euler.vy & 0xfff) - VInfo->OldEuler.vy + 0x4000) & 0xfff;
	DeltaZ = ((VInfo->Euler.vz & 0xfff) - VInfo->OldEuler.vz + 0x4000) & 0xfff;
	VInfo->Euler.vx &= 0xfff;
	VInfo->Euler.vy &= 0xfff;
	VInfo->Euler.vz &= 0xfff;

	if (DeltaX >= 2048)
		DeltaX -= 4096;

	if (DeltaY >= 2048)
		DeltaY -= 4096;

	if (DeltaZ >= 2048)
		DeltaZ -= 4096;

	VInfo->OldEuler = VInfo->Euler;
	bx = BitsRequired(DeltaX);
	by = BitsRequired(DeltaY);
	bz = BitsRequired(DeltaZ);

	if (bx < by)
		bx = by;

	if (bx < bz)
		bx = bz;

	for (i = 0; i < 6; i++)
		if (EulerBits[i] >= bx)
			break;

	PutNBits(VInfo, PrefixCode[i], PrefixBits[i]);
	PutNBits(VInfo, DeltaX, EulerBits[i]);
	PutNBits(VInfo, DeltaY, EulerBits[i]);
	PutNBits(VInfo, DeltaZ, EulerBits[i]);

	if (pSkater->mPhysicsState == VInfo->OldPhysicsState)
		PutNBits(VInfo, 0, 1);
	else
		PutNBits(VInfo, (pSkater->mPhysicsState << 1) | 1, 5);

	VInfo->OldPhysicsState = pSkater->mPhysicsState;

	if (pSkater->mAnim == VInfo->OldAnim)
		PutNBits(VInfo, 0, 1);
	else
		PutNBits(VInfo, (pSkater->mAnim << 1) | 1, 8);

	VInfo->OldAnim = pSkater->mAnim;
	DeltaX = pSkater->mFrame - VInfo->OldFrame;

	if ((unsigned)DeltaX < 4)
		PutNBits(VInfo, DeltaX << 1, 3);
	else
		PutNBits(VInfo, (pSkater->mFrame << 1) | 1, 9);

	VInfo->OldFrame = pSkater->mFrame;
	PutNBits(VInfo, (pSkater->mExtraFlags >> 1) & 1, 1);

	if (!pSkater->m720) {
		PutNBits(VInfo, 0, 1);

		VInfo->OldFlipAngle = 0;
		VInfo->FlipAngleDelta = 0;
	} else {
		PutNBits(VInfo, 1, 1);

		{
			int Delta, Bits;

			Delta = pSkater->m724 - (VInfo->OldFlipAngle + VInfo->FlipAngleDelta);
			Bits = 0;

			if (Delta)
				Bits = BitsRequired(Delta);

			for (i = 0; i < 6; i++)
				if (FlipAngleBits[i] >= Bits)
					break;

			PutNBits(VInfo, PrefixCode[i], PrefixBits[i]);
			PutNBits(VInfo, Delta, FlipAngleBits[i]);

			VInfo->OldFlipAngle = pSkater->m724;
			VInfo->FlipAngleDelta = Delta;
		}
	}

	DeltaX = Xblanks - VInfo->OldXblanks;

	if (DeltaX >= 6) {
		VInfo->OldXblanks = Xblanks - (DeltaX - 5);
		DeltaX = 5;
	} else
		VInfo->OldXblanks = Xblanks;

	PutNBits(VInfo, DeltaX - 2, 2);

	if (HasSoundCmd()) {
		PutNBits(VInfo, 1, 1);

		{
			ESoundCmdType SoundCmdType;
			int SoundNum;

			DequeueSoundCmd(SoundCmdType, SoundNum);

			if (SoundCmdType == SOUNDCMD_PLAY) {
				PutNBits(VInfo, 1, 1);
				PutNBits(VInfo, SoundNum, 8);
			} else if (SoundCmdType == SOUNDCMD_STOP) {
				PutNBits(VInfo, 0, 1);
				PutNBits(VInfo, SoundNum, 8);
			}
		}
	} else
		PutNBits(VInfo, 0, 1);

	if (Splat1 == -1 && Splat2 == Splat1 && TrickOb1 == Splat2 && TrickOb2 == TrickOb1) {
		PutNBits(VInfo, 0, 1);
		return;
	}

	PutNBits(VInfo, 1, 1);

	if (pSkater == MechList) {
		if (Splat1 != -1) {
			PutNBits(VInfo, 1, 1);
			PutNBits(VInfo, Splat1, 2);

			Splat1 = -1;
		} else
			PutNBits(VInfo, 0, 1);
	} else {
		if (Splat2 != -1) {
			PutNBits(VInfo, 1, 1);
			PutNBits(VInfo, Splat2, 2);

			Splat2 = -1;
		} else
			PutNBits(VInfo, 0, 1);
	}

	if (pSkater == MechList) {
		if (TrickOb1 != -1) {
			PutNBits(VInfo, 1, 1);
			PutNBits(VInfo, TrickOb1, 8);

			TrickOb1 = -1;
			return;
		}
	} else {
		if (TrickOb2 != -1) {
			PutNBits(VInfo, 1, 1);
			PutNBits(VInfo, TrickOb2, 8);

			TrickOb2 = -1;
			return;
		}
	}

	PutNBits(VInfo, 0, 1);
}

/* Video_Record__FP6CBruce (0x8006a118) */
void Video_Record(CBruce *b) {
	if (GState == 1)
		Video_Stop(b);
	else
		PutDataChunk(b);
}

/* GetDataChunk__FP6CBrucei (0x8006a158) */
/* Locals, their order and types come from the demo's MAIN.SYM. */
/* ScaleFactor unused */
void GetDataChunk(CBruce *pSkater, int NChunks) {
	int i;
	int DeltaX, DeltaY, DeltaZ;
	SVideoInfo *VInfo;

	if (pSkater == MechList)
		VInfo = &GVInfo1[0];
	else
		VInfo = &GVInfo1[1];

	pSkater->mOldPos = pSkater->mPos;

	for (i = 0; i < 6; i++)
		if (!GetBit(VInfo))
			break;

	pSkater->mPos.vx = VInfo->OldPos.vx * 2 - VInfo->OldPos2.vx + GetNBitsSigned(VInfo, PosBits[i]);
	pSkater->mPos.vy = VInfo->OldPos.vy * 2 - VInfo->OldPos2.vy + GetNBitsSigned(VInfo, PosBits[i]);
	pSkater->mPos.vz = VInfo->OldPos.vz * 2 - VInfo->OldPos2.vz + GetNBitsSigned(VInfo, PosBits[i]);
	VInfo->OldPos2 = VInfo->OldPos;
	VInfo->OldPos = pSkater->mPos;
	pSkater->mPos <<= 12;
	pSkater->mVel = pSkater->mPos - pSkater->mOldPos;

	for (i = 0; i < 6; i++)
		if (!GetBit(VInfo))
			break;

	DeltaX = GetNBitsSigned(VInfo, EulerBits[i]) & 0xfff;
	DeltaY = GetNBitsSigned(VInfo, EulerBits[i]) & 0xfff;
	DeltaZ = GetNBitsSigned(VInfo, EulerBits[i]) & 0xfff;
	VInfo->Euler.vx = (VInfo->OldEuler.vx + DeltaX + 0x4000) & 0xfff;
	VInfo->Euler.vy = (VInfo->OldEuler.vy + DeltaY + 0x4000) & 0xfff;
	VInfo->Euler.vz = (VInfo->OldEuler.vz + DeltaZ + 0x4000) & 0xfff;
	VInfo->OldEuler = VInfo->Euler;

	if (NChunks == 1) {
		RotMatrixYXZ_gte(&VInfo->Euler, &pSkater->mRotMatrix);

		pSkater->mRotMatrix.m[0][1] = -pSkater->mRotMatrix.m[0][1];
		pSkater->mRotMatrix.m[1][1] = -pSkater->mRotMatrix.m[1][1];
		pSkater->mRotMatrix.m[2][1] = -pSkater->mRotMatrix.m[2][1];
		pSkater->FRUVectorsFromRotMatrix();
		M3d_BuildTransform((CSuper *)pSkater);
	}

	if (GetBit(VInfo) == 1)
		pSkater->mPhysicsState = (EPhysicsState)GetNBitsUnsigned(VInfo, 4);

	if (GetBit(VInfo) == 1)
		pSkater->mAnim = GetNBitsUnsigned(VInfo, 7);

	VInfo->OldAnim = pSkater->mAnim;

	if (!GetBit(VInfo))
		pSkater->mFrame = VInfo->OldFrame + GetNBitsUnsigned(VInfo, 2);
	else
		pSkater->mFrame = GetNBitsUnsigned(VInfo, 8);

	VInfo->OldFrame = pSkater->mFrame;

	if (GetBit(VInfo))
		pSkater->mExtraFlags |= 2;
	else
		pSkater->mExtraFlags &= ~2;

	if (!GetBit(VInfo)) {
		VInfo->OldFlipAngle = 0;
		VInfo->FlipAngleDelta = 0;
		pSkater->m724 = 0;
		pSkater->m720 = 0;
	} else {
		pSkater->m720 = 1000;

		for (i = 0; i < 6; i++)
			if (!GetBit(VInfo))
				break;

		if (FlipAngleBits[i]) {
			int Delta = GetNBitsSigned(VInfo, FlipAngleBits[i]);

			pSkater->m724 = VInfo->OldFlipAngle + VInfo->FlipAngleDelta + Delta;
			VInfo->OldFlipAngle = pSkater->m724;
			VInfo->FlipAngleDelta = Delta;
		}
	}

	VInfo->Yblanks += GetNBitsUnsigned(VInfo, 2) + 2;

	if (NChunks == 1) {
		int n = VInfo->Yblanks - Xblanks;

		for (i = 0; i < n; i++)
			PC_FrameWait(1);
	}

	if (GetBit(VInfo)) {
		if (GetBit(VInfo)) {
			int SoundNum = GetNBitsUnsigned(VInfo, 8);

			if (!ReplaySoundLockout)
				SFX_PlayX((ESfx)SoundNum, 0x2000, 0);
		} else {
			int SoundNum = GetNBitsUnsigned(VInfo, 8);

			if (!ReplaySoundLockout)
				SFX_Stop(SoundNum);
		}
	}

	if (GetBit(VInfo)) {
		if (GetBit(VInfo))
			pSkater->BloodSplat(GetNBitsUnsigned(VInfo, 2), 50);

		if (GetBit(VInfo)) {
			int TrickObIndex = GetNBitsUnsigned(VInfo, 8);
			CTrickOb *pTrickOb = TrickObList;

			for (int i = 0; i < TrickObIndex; i++)
				pTrickOb = pTrickOb->mpNext;

			pTrickOb->mpItem = Spool_FindEnviroItem(pTrickOb->mChecksum);
			pTrickOb->mpItem->Flags |= 0x400;
			pTrickOb->mpItem->RGB = 0x202020;
			pTrickOb->mState = 1;

			if (pSkater == MechList)
				pTrickOb->mPlayer = 1;
			else
				pTrickOb->mPlayer = 2;

			TrickPlayer = pTrickOb->mPlayer;
			Trig_SendPulse(Trig_GetLinksPointer(pTrickOb->mNode));
		}
	}

	if (VInfo->OldPos2.vx || VInfo->OldPos2.vy || VInfo->OldPos2.vz) {
		SLineInfo LInfo;
		int Player;
		CVector nearest;
		CVector Start;
		CVector End;

		LInfo.StartCoords.vx = VInfo->OldPos2.vx << 12;
		LInfo.StartCoords.vy = VInfo->OldPos2.vy << 12;
		LInfo.StartCoords.vz = VInfo->OldPos2.vz << 12;
		LInfo.EndCoords.vx = VInfo->OldPos.vx << 12;
		LInfo.EndCoords.vy = VInfo->OldPos.vy << 12;
		LInfo.EndCoords.vz = VInfo->OldPos.vz << 12;
		M3dColij_InitLineInfo(&LInfo);

		LInfo.RecordTriggerZoneHits = 1;
		Player = 2;

		if (pSkater == GSkater[0])
			Player = 1;

		Trig_ResetCPCollisionFlags(Player);

		pCurrentPlayer = pSkater;
		M3dZone_LineToItem(&LInfo, 1);

		pCurrentPlayer = 0;
		Trig_ResetCPExecutedFlags(Player);

		Start.vx = VInfo->OldPos2.vx << 12;
		Start.vy = VInfo->OldPos2.vy << 12;
		Start.vz = VInfo->OldPos2.vz << 12;
		End.vx = VInfo->OldPos.vx << 12;
		End.vy = VInfo->OldPos.vy << 12;
		End.vz = VInfo->OldPos.vz << 12;
		CBody *pBody = M3dColij_LineToSphere(Start, End, nearest, PowerUpList, 0);

		if (pBody)
			pBody->Die();
	}

	switch (pSkater->mAnim) {
		case 0x36:
			if (pSkater->mFrame >= 0x70)
				goto On;
		case 0x37:
		case 0x2c:
		case 0x2e:
			pSkater->BoardOff();
			break;
		default:
		On:
			pSkater->BoardOn();
	}

	if (Stats[pSkater->mSkaterIndex].ComboID == 0x4000) {
		if (pSkater->mAnim != 0x3f)
			pSkater->mSubObjectDisplayMask |= 0x1000;
		else
			pSkater->mSubObjectDisplayMask &= ~0x1000;
	}

	if (NChunks != 1)
		VInfo->Yblanks = Xblanks;
}

/* Video_Playback__FP6CBruce (0x8006a930) */
/* Fuzzy demo twin; locals and blocks from its MAIN.SYM. THPS1's version calls
   Platform_HandleBounce with no argument. */
void Video_Playback(CBruce *pSkater) {
	SVideoInfo *VInfo;

	CamPt_SetClosestCamera(pSkater);

	if (pSkater == MechList)
		VInfo = &GVInfo1[0];
	else
		VInfo = &GVInfo1[1];

	if (Status == 20 || Status == 22 || Status == 21) {
		if (!VInfo->FramesLeft) {
			RestartCompFrig();
			Video_Start(pSkater, 2, 1, 0x7fffffff);
		}
	}

	if (VInfo->FramesLeft) {
		int IsAGrind;

		GetDataChunk(pSkater, 1);

		VInfo->FramesLeft--;
		IsAGrind = ABS(Mips_DotProd2(-pSkater->mFront.vz, pSkater->mVel.vx, pSkater->mFront.vx,
		                             pSkater->mVel.vz)) < 40000;
		int st = pSkater->mPhysicsState;

		if (st == 4 && (IsAGrind || pSkater->mAnim == 58) && BitCount < 200) {
			CSVector Angles;

			{
				short x = VInfo->Euler.vx, y = VInfo->Euler.vy, z = VInfo->Euler.vz;

				Angles.vx = x;
				Angles.vy = y;
				Angles.vz = z;
			}

			Bit_SetSparkTrajectory(Angles);
			CSVector cone;

			cone.vx = 450;
			cone.vy = 900;
			cone.vz = 0;
			Bit_SetSparkTrajectoryCone(cone);

			UseCacheForBits = 0;
			CVector SparkPos;

			/* retail stores vz, vx, vy (not the header ctor's order) */
			SparkPos.vz = 0;
			SparkPos.vx = 0;
			SparkPos.vy = 0x19000;

			for (int i = 0; i < 9; i++)
				new CGrindSpark((CBody *)pSkater, SparkPos, -40, 4096, 5);

			st = pSkater->mPhysicsState;
			UseCacheForBits = 1;
		}

		if (st || ABS(pSkater->mPos.vy - pSkater->m2E4) > 64) {
			pSkater->m2E4 = pSkater->mPos.vy;
			pSkater->DoShadowCheck();
		} else {
			pSkater->mShadowPos.vx = pSkater->mPos.vx;
			pSkater->mShadowPos.vz = pSkater->mPos.vz;
		}

		pSkater->GlowUpdate();
		Platform_HandleBounce();
		pSkater->HandleWater();
	} else {
		if ((uint)(VInfo->ReplayHighlight - 1) < 4 &&
		    VInfo->Highlights[VInfo->ReplayHighlight].Score) {
			int StartFrame = VInfo->Highlights[VInfo->ReplayHighlight].FromFrame;
			int EndFrame = VInfo->Highlights[VInfo->ReplayHighlight].ToFrame;

			if (EndFrame != 0x7fffffff)
				VInfo->FramesLeft = EndFrame - StartFrame + 1;

			{
				int n = GVideoReplay;

				VInfo->ReplayHighlight++;
				GVideoReplay = n + 1;
			}

			Video_SyncObsIfNeeded();
		} else {
			Video_Stop(pSkater);

			if (GNumberOfPlayers == 1 || pSkater == MechList)
				Front_SwitchOnVideoScreen();
		}
	}
}

/* Video_HandleStepping__Fv (0x8006ac84) */
void Video_HandleStepping() {
	int i, v;

	if (GVInfo1[0].VideoMode != 2 || DemoTimer == 0) {
		if (DemoTimer != 0)
			return;

		for (i = 0; i < 2; i++) {
			if (Pad[i].Grind.Triggered || Pad[i].Flip.Triggered ||
			    Pad[i].SpinLeft.Triggered || Pad[i].TapSpinLeft.Triggered ||
			    Pad[i].SpinRight.Triggered || Pad[i].TapSpinRight.Triggered ||
			    Pad[i].Left.Triggered || Pad[i].Right.Triggered ||
			    Pad[i].Up.Triggered || Pad[i].Down.Triggered ||
			    Pad[i].Grab.Triggered || Pad[i].Camera.Triggered)
				Pad[i].Ollie.Triggered = 1;
		}

		return;
	}

	if (Status == 20 || Status == 22 || Status == 21)
		return;

	if (Pad[0].Flip.Triggered ||
	    (GNumberOfPlayers == 2 && Pad[1].Flip.Triggered)) {
		v = GVInfo1[0].PlaybackFrozen;
		Pad[0].Flip.Triggered = 0;
		Pad[1].Flip.Triggered = 0;
		v ^= 1;
		GVInfo1[0].PlaybackFrozen = v;
		GameFrozen = v;
		MusicStopped = v;
	}

	if (GVInfo1[0].PlaybackFrozen == 0)
		return;

	GameFrozen = 1;

	if (Pad[0].Grab.Pressed ||
	    (GNumberOfPlayers == 2 && Pad[1].Grab.Pressed)) {
		if (DAT_800d1df0 != 0) {
			DAT_800d1df0--;
			return;
		}

		DAT_800d1df0 = 6;
		GameFrozen = 0;
		return;
	}

	if (Pad[0].Grind.Pressed ||
	    (GNumberOfPlayers == 2 && Pad[1].Grind.Pressed)) {
		if (DAT_800d1df0 != 0) {
			DAT_800d1df0--;
			return;
		}

		DAT_800d1df0 = 1;
		GameFrozen = 0;
		return;
	}

	DAT_800d1df0 = 0;
	return;
}

/* FUN_8006af5c (0x8006af5c) */
void FUN_8006af5c(void) {}

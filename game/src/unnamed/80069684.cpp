/* Video_Start__FP6CBruce10EVideoModeii (0x80069684) */
#include "game/video.h"
#include "game/bruce.h"

extern SVideoInfo GVInfo1[];
extern SVideoInfo GVInfo1[2];
extern CBruce *MechList[2];

extern int HLStartCalled;

extern int Xblanks;

extern "C" void FUN_80068d88();
extern "C" void FUN_80068ed4();
extern "C" void FUN_80068f2c();

void Video_Start(CBruce *a, EVideoMode b, int c, int d) {
	SVideoInfo *s;
	int i, j, y;
	ulong *x;
	ulong *q;
	int g;

	if (a == MechList[0])
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

/* FUN_800440ec (0x800440ec) */
/* C++ (OB.CPP): a body's per-frame AI step (Ob_AI). EveryFrame every frame; AI only every
   mAICount frames, tweening position and angles between the AI frames. The counter at +0xee has no
   name in ob.h yet (padee[0]). */
#include "game/ob.h"

extern "C" void FUN_800440ec(CBody *p) {
	CVector t;
	CVector r1;
	int n1, n2;
	CSVector *s0;
	CSuper *q = (CSuper *)p;

	if (p->mFlags & 2) {
		q->UpdateFrame();
		p->EveryFrame();

		if (p->mAICount < 2) {
			q->mNewFrame = q->mFrame;
			p->AI();

			q->mOldFrame = q->mFrame;
			q->mOldAnim = q->mAnim;
			q->mOldAnimDir = q->mAnimDir;
			return;
		}
	} else {
		p->EveryFrame();

		if (p->mAICount < 2) {
			p->AI();
			return;
		}
	}

	p->padee[0]++;

	if ((uchar)p->padee[0] >= p->mAICount) {
		p->padee[0] = 0;
		p->mPos = p->mNewPos;
		p->mOldPos = p->mPos;
		p->mAngles = p->mNewAngles;
		p->mOldAngles = p->mAngles;

		if (p->mFlags & 2) {
			q->mNewFrame = q->mFrame;
			p->AI();

			q->mOldFrame = q->mFrame;
			q->mOldAnim = q->mAnim;
			q->mOldAnimDir = q->mAnimDir;
		} else {
			p->AI();
		}

		p->mNewPos = p->mPos;
		p->mPos = p->mOldPos;
		r1 = p->mNewPos - p->mOldPos;
		n1 = p->mAICount;
		t = r1 / n1;
		p->mDPos = t;
		p->mNewAngles = p->mAngles;
		p->mAngles = p->mOldAngles;
		p->mDAngles = p->mNewAngles - p->mOldAngles;

		if (p->mDAngles.vx < -2048)
			p->mDAngles.vx += 4096;

		if (p->mDAngles.vx > 2048)
			p->mDAngles.vx -= 4096;

		if (p->mDAngles.vy < -2048)
			p->mDAngles.vy += 4096;

		if (p->mDAngles.vy > 2048)
			p->mDAngles.vy -= 4096;

		if (p->mDAngles.vz < -2048)
			p->mDAngles.vz += 4096;

		if (p->mDAngles.vz > 2048)
			p->mDAngles.vz -= 4096;

		n2 = p->mAICount;
		p->mDAngles /= n2;
		return;
	}

	p->mPos += p->mDPos;

	s0 = &p->mAngles;
	*s0 += p->mDAngles;
	s0->Mask();
}

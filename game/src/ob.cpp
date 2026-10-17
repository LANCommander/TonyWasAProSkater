/* OB.CPP: the world-item hierarchy CItem -> CBody -> CSuper (allocation, object lists,
   suspending, shadows, animation) and the body-list AI loops (declarations in game/ob.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/ob.h"
#include "game/class.h"
#include "game/mem.h"
#include "game/bit.h"
#include "game/m3d.h"
#include "game/globals.h"
#include "game/physics.h"
#include "game/spool.h"
#include "game/utils.h"
#include "game/bruce.h"
#include "pc_video.h"

extern SLight M3d_DefaultLight;
extern "C" char __vt_5CItem[];
extern "C" void __dl__5CItemPv(void *p);
/* objects keep moving as far away as Options > Graphics > Draw Distance shows them */
#define SUSPEND_DISTANCE PC_DrawSuspendDistance(SuspendDistance)
extern "C" void FUN_800440ec(CBody *);
extern short DAT_800d19b8;
extern short DAT_800d19ba;
extern short DAT_800d19bc;

/* __nw__6CClassUi (0x800241dc) */
/* CClass's operator new: allocates from heap 0 (asserting on failure) and zeroes the
   block a word at a time. */
void *CClass::operator new(__SIZE_TYPE__ size) throw() {
	void *p = Mem_NewX(size, 0, 1);
	int *q = (int *)p;

	for (uint i = 0; i < (size + 3) >> 2; i++)
		*q++ = 0;

	return p;
}

/* __nw__5CItemUi (0x800434d4) */
/* CItem's operator new: allocates from heap 0 (asserting on failure) and zeroes the
   block a word at a time. */
void *CItem::operator new(__SIZE_TYPE__ size) throw() {
	void *p = Mem_NewX(size, 0, 1);
	int *q = (int *)p;
	uint words = (size + 3) >> 2;

	for (uint i = 0; i < words; i++)
		*q++ = 0;

	return p;
}

/* __dl__5CItemPv (0x80043530) */
/* CItem's operator delete: frees through the memory manager. */
void CItem::operator delete(void *p) {
	Mem_Delete(p);
}

/* __5CItem (0x80043550) */
/* A new item starts at unit scale, lit by the default light. */
CItem::CItem() {
	mScale.vx = 4096;
	mScale.vy = 4096;
	mScale.vz = 4096;
	mpLight = &M3d_DefaultLight;
}

/* FUN_80043584 (0x80043584) */
/* C++: CItem's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail has. */
/* egcs-ABI call, as retail makes it */
extern "C" void FUN_80043584(CItem *p, int f) {
	p->__vptr = __vt_5CItem;

	if (f & 1)
		__dl__5CItemPv(p);
}

/* InitItem__5CItemPc (0x800435b8) */
/* Binds the item to the named spooled region, model 0. */
void CItem::InitItem(char *name) {
	mRegion = Spool_FindRegion(name);
	mModel = 0;
}

/* Flash__5CItemiUcUcUc (0x800435e8) */
void CItem::Flash(int frames, uchar r, uchar g, uchar b) {
	if (frames == 0) {
		mFlashTimer = 0;
		mRGB = (b << 16) | (g << 8) | r;
		mFlags |= 1024;
	} else {
		if (frames < 0) {
			mFlashTimer = -frames;
			mFlash_dR = (r - 128) / mFlashTimer;
			mFlash_dG = (g - 128) / mFlashTimer;
			mFlash_dB = (b - 128) / mFlashTimer;
			mRGB = 0x808080;
			mFlags |= 1024;
			return;
		}

		mFlash_dR = (128 - r) / frames;
		mFlash_dG = (128 - g) / frames;
		mFlash_dB = (128 - b) / frames;
		mFlashTimer = frames + 1;

		if (!(mStoredRGB & 0x01000000)) {
			mStoredRGB = (mRGB & 0x00ffffff) | 0x01000000;

			if (mFlags & 1024)
				mStoredRGB |= 0x02000000;
		}

		mRGB = (b << 16) | (g << 8) | r;
		mFlags |= 1024;
	}
}

/* FlashUpdate__5CItem (0x80043728) */
void CItem::FlashUpdate() {
	if (mFlashTimer) {
		int r, g, b;

		mFlashTimer--;
		r = (mRGB & 255);
		g = ((mRGB >> 8) & 255);
		b = ((mRGB >> 16) & 255);
		r += mFlash_dR;
		g += mFlash_dG;
		b += mFlash_dB;
		mRGB = ((b & 255) << 16) | ((g & 255) << 8) | (r & 255);
		mFlags |= 1024;

		if (mFlashTimer == 0) {
			if (mStoredRGB & 0x02000000)
				mFlags |= 1024;
			else
				mFlags &= ~1024;

			mRGB = mStoredRGB;
			mStoredRGB = 0;
		}
	}
}

/* __5CBody (0x800437d4) */
CBody::CBody() : mFric(1, 1, 1), mAngFric(1, 1, 1) {
	mPushVal = 10;
	m98 = 0;
	mNode = 0xffff;
	mShadowScale = 50;
	mShadowThreshold = 300;
	mCBodyFlags |= CBODY_AUTO_SUSPEND | CBODY_FLAG_10;
	mRMinor = 50;
}

/* _._5CBody (0x80043860) */
/* Deletes the body's shadow bit; CItem's destructor does the rest. */
CBody::~CBody() {
	if (mpShadow)
		delete mpShadow;
}

/* AttachTo__5CBodyPP5CBody (0x800438dc) */
/* Pushes the body onto the front of a doubly linked object list. */
void CBody::AttachTo(CBody **pp_list) {
	mPreviousItem = 0;
	mNextItem = *pp_list;
	*pp_list = this;

	if (mNextItem)
		mNextItem->mPreviousItem = this;
}

/* DeleteFrom__5CBodyPP5CBody (0x80043908) */
/* Unlinks the body from a list (taking it off the suspended list first if it is
   suspended and the list is another one). */
void CBody::DeleteFrom(CBody **pp_list) {
	if ((mCBodyFlags & CBODY_SUSPENDED) && pp_list != &SuspendedList)
		UnSuspend();

	if (mNextItem)
		mNextItem->mPreviousItem = mPreviousItem;

	if (mPreviousItem)
		mPreviousItem->mNextItem = mNextItem;

	if (*pp_list == this)
		*pp_list = (CBody *)mNextItem;
}

/* SquirtPos__5CBodyPs (0x800439ac) */
/* Reads a whole-unit position (three ints) from object data into the position and
   tween position (as 20.12 fixed point); returns the data pointer past it. */
short *CBody::SquirtPos(short *p_info) {
	int *p = (int *)p_info;

	mPos.vx = mTweenPos.vx = p[0] << 12;
	mPos.vy = mTweenPos.vy = p[1] << 12;
	mPos.vz = mTweenPos.vz = p[2] << 12;
	return (short *)(p + 3);
}

/* SquirtAngles__5CBodyPs (0x800439ec) */
/* Reads the three angles from object data; returns the data pointer past them. */
short *CBody::SquirtAngles(short *p_info) {
	mAngles.vx = *p_info++;
	mAngles.vy = *p_info++;
	mAngles.vz = *p_info++;
	return p_info;
}

/* Suspend__5CBodyPP5CBody (0x80043a14) */
/* Frees the body's extras (virtual DeleteStuff), then moves it from its list onto
   the suspended list, remembering the list to return to. */
void CBody::Suspend(CBody **pp_list) {
	DeleteStuff();

	mppOriginalList = pp_list;
	DeleteFrom(pp_list);
	AttachTo(&SuspendedList);

	mCBodyFlags |= CBODY_SUSPENDED;
}

/* UnSuspend__5CBody (0x80043a80) */
/* Moves a suspended body from the suspended list back onto the list it came from. */
void CBody::UnSuspend() {
	if (!(mCBodyFlags & CBODY_SUSPENDED))
		return;

	DeleteFrom(&SuspendedList);
	AttachTo(mppOriginalList);

	mCBodyFlags &= ~CBODY_SUSPENDED;
}

/* ShadowOn__5CBody (0x80043ae0) */
/* Turns the body's shadow on. */
void CBody::ShadowOn() {
	mCBodyFlags |= CBODY_SHADOW;
}

/* KillShadow__5CBody (0x80043af4) */
/* Turns the body's shadow off and deletes its shadow bit. */
void CBody::KillShadow() {
	mCBodyFlags &= ~CBODY_SHADOW;

	if (mpShadow) {
		delete mpShadow;

		mpShadow = 0;
	}
}

/* UpdateShadow__5CBody (0x80043b50) */
/* Exact demo twin; written from its MAIN.SYM (up/trans in the flag block). The
   shadow quad comes from CBit's pool allocator (operator new, null-checked). */
void CBody::UpdateShadow() {
	if (mCBodyFlags & CBODY_SHADOW) {
		SVECTOR up;
		int trans;

		if (!mpShadow) {
			UseCacheForBits = 0;
			CQuadBit *p = new CQuadBit;

			mpShadow = p;
			UseCacheForBits = 1;
			p->SetTexture("Shadow", 0);
			mpShadow->FlagOn(4);
			mpShadow->SetSemiTransparent();

			mpShadow->mFrigDeltaZ = 32;
			mpShadow->mProtected = 1;
		}

		if (mShadowNormal.vy < -128) {
			up.vx = mShadowNormal.vx;
			up.vy = mShadowNormal.vy;
			up.vz = mShadowNormal.vz;
			mpShadow->OrientUsing(mShadowPos, up, mShadowScale, mShadowScale, 0);
		}

		trans = ((mShadowThreshold - mShadowDist) << 7) / mShadowThreshold;

		if (trans < 0)
			trans = 0;

		trans /= 6;
		mpShadow->SetTransparency(trans);
	} else {
		KillShadow();
	}
}

/* Die__5CBody (0x80043cac) */
/* Marks the body dead (once). */
void CBody::Die() {
	if (!IsDead())
		mCBodyFlags |= CBODY_DEAD;
}

/* IsDead__C5CBody (0x80043ce8) */
/* Whether the body has been marked dead. */
int CBody::IsDead() const {
	return (mCBodyFlags & CBODY_DEAD) != 0;
}

/* FUN_80043cfc (0x80043cfc) */
int FUN_80043cfc(void) {
	return 1;
}

/* __6CSuper (0x80043d04) */
/* C++: constructor of a CBody-derived class (vtable 0x800a0574). */
CSuper::CSuper() {
	mExtraFlags = 0;
	mAnimSpeed = 0x10000;
	mNumFrames = 1;
	mAnimFinished = 1;
	mFlags |= 2;
}

/* _._6CSuper (0x80043d5c) */
/* Frees the animation buffers the super item allocated; CBody's destructor does the
   rest. */
CSuper::~CSuper() {
	if (mpPoseBuffer)
		Mem_Delete(mpPoseBuffer);

	if (mpJoints)
		Mem_Delete(mpJoints);

	if (mpDecompressedFrame)
		Mem_Delete(mpDecompressedFrame);

	if (mpCalculationOrder)
		Mem_Delete(mpCalculationOrder);
}

/* RunAnim__6CSuperiiii (0x80043e0c) */
/* Demo twin (same shape); locals from its MAIN.SYM. Starts an animation: clamps the
   anim index to the model's table, then the from/to frames (-1 = last), and sets the play direction.
   The direction byte is signed so its -1 shares the register of the "from == -1" test. */
void CSuper::RunAnim(int a, int from, int to, int to2) {
	mAnim = a;

	if (mAnim >= *PSXRegion[mRegion].pAnimFile)
		mAnim = 46;

	mNumFrames = ((unsigned char *)PSXRegion[mRegion].pAnimFile)[mAnim * 8 + 8];

	if (from == -1)
		from = mNumFrames - 1;

	if (to == -1)
		to = mNumFrames - 1;

	if (from < 0)
		from = 0;

	if (to < 0)
		to = 0;

	if (from >= mNumFrames)
		from = mNumFrames - 1;

	if (to >= mNumFrames)
		to = mNumFrames - 1;

	mAnimMode = 0;

	if (from >= to) {
		if (to < from)
			mAnimDir = -1;
		else
			mAnimDir = 0;
	} else
		mAnimDir = 1;

	mFrame = from;
	mTargetFrame = to;
	mTargetFrame2 = to2;
	mFrameFrac = 0;
	mAnimFinished = (short)from == (signed char)to;
}

/* CycleAnim__6CSuperiSc (0x80043f24) */
/* Starts animation `anim` looping (mode 1) in direction `dir` from frame 0, taking
   its frame count from the region's animation file. */
void CSuper::CycleAnim(int anim, signed char dir) {
	uchar num_frames;

	mAnim = anim;
	num_frames = ((uchar *)PSXRegion[mRegion].pAnimFile)[mAnim * 8 + 8];
	mAnimMode = 1;
	mAnimDir = dir;
	mFrame = 0;
	mFrameFrac = 0;
	mAnimFinished = 0;
	mNumFrames = num_frames;
}

/* UpdateFrame__6CSuper (0x80043f70) */
/* Advances the animation frame by mAnimSpeed (scaled by TimeScale) in 16.16 fixed
   point in direction mAnimDir. Mode 0 (play to target) clamps at mTargetFrame, ping-ponging to
   mTargetFrame2 when one is set and flagging mAnimFinished otherwise; mode 1 (cycle) wraps the
   frame within mNumFrames. */
void CSuper::UpdateFrame() {
	int v, a, c, d, s;
	unsigned char ub;

	if (mAnimMode == 0) {
		c = mAnimDir;

		if ((c == 1 && (a = (unsigned char)mTargetFrame, mFrame >= mTargetFrame)) ||
		    (c == -1 && (a = (unsigned char)mTargetFrame, mFrame <= mTargetFrame))) {
			if ((ub = mTargetFrame2, mTargetFrame2 > 0)) {
				mTargetFrame = ub;
				mTargetFrame2 = a;
				mAnimDir = -(unsigned char)mAnimDir;
			} else
				mAnimFinished = 1;
		}
	}

	v = (mFrame << 16) | mFrameFrac;
	d = mAnimDir;
	a = (mAnimSpeed * TimeScale) >> 8;

	if (d == 1)
		v += a;

	if (d == -1)
		v -= a;

	s = v >> 16;
	mFrame = s;
	mFrameFrac = v;

	switch ((unsigned char)mAnimMode) {
		case 0:
			if ((d == 1 && !(s < mTargetFrame)) || (d == -1 && !(mTargetFrame < s)))
				mFrame = (signed char)(unsigned char)mTargetFrame;

			break;
		case 1:
			if (s >= mNumFrames)
				mFrame = 0;

			if (mFrame < 0)
				mFrame = mNumFrames - 1;

			break;
	}
}

/* FUN_800440ec (0x800440ec) */
/* : a body's per-frame AI step (Ob_AI). EveryFrame every frame; AI only every
   mAICount frames, tweening position and angles between the AI frames. The counter at +0xee has no
   name in ob.h yet (padee[0]). */
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

/* Ob_ClearInputFlags__FP5CBody (0x80044484) */
void Ob_ClearInputFlags(CBody *b) {
	CBody *n;

	if (b) {
		n = (CBody *)b->mNextItem;

		while (1) {
			b->mInputFlags &= 1;
			b = n;

			if (!b)
				break;

			n = (CBody *)b->mNextItem;
		}
	}
}

/* Ob_AI__FPP5CBody (0x800444bc) */
/* : runs a body list's AI: flash update, deferred deletes (flag 0x40, done on the
   second frame), the distance to the nearest skater, then suspend or AI + shadow. */
void Ob_AI(CBody **a) {
	CBody *m;
	CBody *n;
	CVector *v;
	unsigned d, t;

	m = *a;

	if (!m)
		return;

	n = (CBody *)m->mNextItem;

	for (;;) {
		m->FlashUpdate();

		if (m->mCBodyFlags & CBODY_DEAD) {
			if (m->mCBodyFlags & CBODY_DELETE_PENDING) {
				if (m)
					delete m;
			} else
				m->mCBodyFlags = m->mCBodyFlags | CBODY_DELETE_PENDING;
		} else {
			v = &m->mPos;
			d = Utils_CrapDist(MechList->mPos, *v);

			if (GNumberOfPlayers == 2) {
				t = Utils_CrapDist(MechList->mNextItem->mPos, *v);

				if (t < d)
					d = t;
			}

			m->mPlayerDist = d;

			if ((m->mCBodyFlags & CBODY_AUTO_SUSPEND) && SUSPEND_DISTANCE < d) {
				m->Suspend(a);
			} else {
				FUN_800440ec(m);
				m->UpdateShadow();
			}
		}

		m = n;

		if (n)
			n = (CBody *)n->mNextItem;
		else
			break;
	}
}

/* Ob_QuickAI__FPP5CBody (0x80044620) */
/* Fuzzy demo twin; locals from its MAIN.SYM. Runs AI on every body in the list;
   dead bodies are deleted on the second pass (CBODY_DELETE_PENDING marks the first). */
void Ob_QuickAI(CBody **ppBody) {
	CBody *pBody;
	CBody *pNext;

	/* the exit test loads pNext (probably a list-walk macro), so cc1 can't rotate it into a
	   simple bottom test and `delete` keeps its null check */
	for (pBody = *ppBody; pBody ? (pNext = (CBody *)pBody->mNextItem, 1) : 0; pBody = pNext) {
		if (pBody->mCBodyFlags & CBODY_DEAD) {
			if (pBody->mCBodyFlags & CBODY_DELETE_PENDING)
				delete pBody;
			else
				pBody->mCBodyFlags |= CBODY_DELETE_PENDING;
		} else {
			pBody->AI();
		}
	}
}

/* Ob_MaybeUnSuspendOrCull__Fv (0x800446d4) */
void Ob_MaybeUnSuspendOrCull() {
	CBody *b = SuspendedList;
	CBody *n;

	if (!b)
		return;

	n = (CBody *)b->mNextItem;

	for (;;) {
		if (b->mCBodyFlags & CBODY_AUTO_SUSPEND) {
			unsigned d = Utils_CrapDist(MechList->mPos, b->mPos);

			if (GNumberOfPlayers == 2) {
				unsigned d2 = Utils_CrapDist(MechList->mNextItem->mPos, b->mPos);

				if (d2 < d)
					d = d2;
			}

			if (!(SUSPEND_DISTANCE < d))
				b->UnSuspend();
		}

		b = n;

		if (n)
			n = (CBody *)n->mNextItem;
		else
			break;
	}
}

/* FUN_800447ac (0x800447ac) */
void FUN_800447ac(void) {
	ZeroVector.vx = 0;
	ZeroVector.vy = 0;
	ZeroVector.vz = 0;
	DAT_800d19b8 = 0;
	DAT_800d19ba = 0;
	DAT_800d19bc = 0;
}

/* FUN_800447d4 (0x800447d4) */
void FUN_800447d4(void) {}

/* FUN_800447dc (0x800447dc) */
void FUN_800447dc(void) {}

/* FUN_800447e4 (0x800447e4) */
void FUN_800447e4(void) {}

/* FUN_800447ec (0x800447ec) */
void FUN_800447ec(void) {}

/* The world-item hierarchy (OB.H / OB.CPP): CItem -> CBody -> CSuper. CBaddy, CBruce, CPlatform,
   CCar, CTape, CPowerUp, CBackground, CCamera ... derive from CBody/CSuper (their own headers).

   egcs ABI: CItem is the root polymorphic class, so the vptr sits after its data at +0x48 (not
   declared here: the port build inserts it, egcs places it there). CItem's own vtable
   (0x800a05f4) has a single slot, its destructor; CBody adds slots 1..6 (vtable 0x800a05b4),
   CSuper overrides only the destructor (0x800a0574).

   Retail layouts are the demo's (MAIN.SYM) shifted: CBody grew 0x18 bytes at +0x54, one byte at
   +0x84 and 0x14 bytes around +0x98..0xb4; CSuper lost the demo's ping-pong/start-frame fields
   and the trailing anim-order data. See build/hdr_notes/ob.md. */
#ifndef GAME_OB_H
#define GAME_OB_H

#pragma interface /* see class.h */

/* CPPPSX doesn't define __cplusplus, so the SDK headers' own extern "C" guards are inactive under
   egcs: wrap them, as the TUs do */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/types.h"
#include "game/vector.h"

struct SLight;
struct SMatrix;
struct SJoint;
struct SLink;
class CQuadBit;

/* the world item (vtable 0x800a05f4; not the CREDITS or front-end classes also named CItem) */
class CItem {
  public:
	ushort mFlags;             /* 0x00 */
	ushort mInquiry;           /* 0x02 */
	CVector mPos;              /* 0x04 */
	CSVector mAngles;          /* 0x10 */
	ushort mModel;             /* 0x16 */
	char mDummyFrame;          /* 0x18 */
	uchar mTintIndex;          /* 0x19 */
	uchar mDummyAnim;          /* 0x1a */
	uchar mRegion;             /* 0x1b */
	CItem *mNextItem;          /* 0x1c */
	uint mRGB;                 /* 0x20 */
	CSVector mScale;           /* 0x24 */
	uint mTRN;                 /* 0x2c */
	CItem *mPreviousItem;      /* 0x30 */
	uint mStoredRGB;           /* 0x34 */
	uchar mPredatorEffectType; /* 0x38 */
	uchar mPredatorEffectFade; /* 0x39 */
	ushort mType;              /* 0x3a */
	SLight *mpLight;           /* 0x3c */
	ushort mFlashTimer;        /* 0x40 */
	short mFlash_dR;           /* 0x42 */
	short mFlash_dG;           /* 0x44 */
	short mFlash_dB;           /* 0x46 */
	/* 0x48: vptr; sizeof(CItem) == 0x4c */

	static void *operator new(uint size) throw(); /* 0x800434d4 */
	void operator delete(void *);                 /* 0x80043530 */
	CItem();                                      /* 0x80043550 */
	virtual ~CItem();                             /* slot 0: 0x80043584 */
	void InitItem(char *);                        /* 0x800435b8 */
	void Flash(int, uchar, uchar, uchar);         /* 0x800435e8 */
	void FlashUpdate();                           /* 0x80043728 */

	void Hide() {
		mFlags &= ~1;
	}
	void Show() {
		mFlags |= 0x41;
	}
};

class CBody : public CItem {
  public:
	CBody **mppOriginalList; /* 0x04c */
	ushort mInputFlags;      /* 0x050 */
	ushort mCBodyFlags;      /* 0x052 */
	char pad54[0x6c - 0x54];
	CVector mVel;       /* 0x06c */
	CVector mAcc;       /* 0x078 */
	uchar m84;          /* 0x084 */
	CFriction mFric;    /* 0x085 */
	CSVector mAngVel;   /* 0x088 */
	CSVector mAngAcc;   /* 0x08e */
	CFriction mAngFric; /* 0x094 */
	char pad97[0x98 - 0x97];
	short m98; /* 0x098 */
	char pad9a[0xa8 - 0x9a];
	CSVector mNormal; /* 0x0a8 */
	ushort mSeed;     /* 0x0ae */
	char padb0[0xb4 - 0xb0];
	uint mEllipsoidRGB;         /* 0x0b4 */
	ushort mEllipsoidScale;     /* 0x0b8 */
	short mEllipsoidOTPushback; /* 0x0ba */
	CVector mShadowPos;         /* 0x0bc */
	CSVector mShadowNormal;     /* 0x0c8 */
	char padce[0xd0 - 0xce];
	CQuadBit *mpShadow;     /* 0x0d0 */
	ushort mShadowScale;    /* 0x0d4 */
	short mShadowDist;      /* 0x0d6 */
	short mShadowThreshold; /* 0x0d8 */
	char padda[0xdc - 0xda];
	int mPushVal;       /* 0x0dc */
	ushort mRMinor;     /* 0x0e0 */
	ushort mNode;       /* 0x0e2 */
	ushort mCollision;  /* 0x0e4 */
	short mHealth;      /* 0x0e6 */
	uint mPlayerDist;   /* 0x0e8 */
	uchar mAIFrequency; /* 0x0ec */
	uchar mAICount;     /* 0x0ed */
	char padee[0xf0 - 0xee];
	CVector mOldPos;     /* 0x0f0 */
	CVector mTweenPos;   /* 0x0fc */
	CVector mNewPos;     /* 0x108 */
	CVector mDPos;       /* 0x114 */
	CSVector mOldAngles; /* 0x120 */
	CSVector mNewAngles; /* 0x126 */
	CSVector mDAngles;   /* 0x12c */
	char pad132[0x134 - 0x132];
	/* sizeof(CBody) == 0x134 */

	CBody();                      /* 0x800437d4 */
	virtual ~CBody();             /* slot 0: 0x80043860 */
	void AttachTo(CBody **);      /* 0x800438dc */
	void DeleteFrom(CBody **);    /* 0x80043908 */
	short *SquirtPos(short *);    /* 0x800439ac */
	short *SquirtAngles(short *); /* 0x800439ec */
	void Suspend(CBody **);       /* 0x80043a14 */
	void UnSuspend();             /* 0x80043a80 */
	void ShadowOn();              /* 0x80043ae0 */
	void KillShadow();            /* 0x80043af4 */
	void UpdateShadow();          /* 0x80043b50 */
	virtual void Die();           /* slot 1: 0x80043cac */
	int IsDead() const;           /* 0x80043ce8 */
	/* slots 2..6: names and order from the demo's vtable; the empty bodies were inline in OB.H */
	virtual void AI() {}                        /* slot 2: 0x800447ec */
	virtual void EveryFrame() {}                /* slot 3: 0x800447e4 */
	virtual void DoLightingSetup() {}           /* slot 4: 0x800447dc */
	virtual int Hit(int, const CVector &, int); /* slot 5: 0x80043cfc (returns 1) */
	virtual void DeleteStuff() {}               /* slot 6: 0x800447d4 */
};

class CSuper : public CBody {
  public:
	uint mExtraFlags;             /* 0x134 */
	ushort *mpCalculationOrder;   /* 0x138 */
	SMatrix *mpDecompressedFrame; /* 0x13c */
	ushort mRoot;                 /* 0x140 */
	uchar mDecompressedAnim;      /* 0x142 */
	char mDecompressedFrame;      /* 0x143 */
	short mFrame;                 /* 0x144 */
	ushort mAnim;                 /* 0x146 */
	uchar mAnimMode;              /* 0x148 */
	signed char mAnimDir;         /* 0x149 */
	signed char mTargetFrame;     /* 0x14a */
	signed char mTargetFrame2;    /* 0x14b: compared with -1 (physics/handlejump: lb) */
	ushort mFrameFrac;            /* 0x14c */
	uchar mNumFrames;             /* 0x14e */
	uchar mAnimFinished;          /* 0x14f */
	int mAnimSpeed;               /* 0x150 */
	short mOldFrame;              /* 0x154 */
	short mNewFrame;              /* 0x156 */
	uchar mOldAnim;               /* 0x158 */
	char mOldAnimDir;             /* 0x159 */
	uchar mShadowLights;          /* 0x15a */
	char pad15b[0x15c - 0x15b];
	SVECTOR *mpShadowSource;    /* 0x15c */
	CVECTOR *mpShadowRGB;       /* 0x160 */
	MATRIX mTransform;          /* 0x164 */
	SMatrix *mpPoseBuffer;      /* 0x184 */
	SJoint *mpJoints;           /* 0x188 */
	SLink *mpLinks;             /* 0x18c */
	short *mpHierarchy;         /* 0x190 */
	uint mSubObjectDisplayMask; /* 0x194 */
	/* sizeof(CSuper) == 0x198 */

	CSuper();                         /* 0x80043d04 */
	virtual ~CSuper();                /* slot 0: 0x80043d5c */
	void RunAnim(int, int, int, int); /* 0x80043e0c */
	void CycleAnim(int, signed char); /* 0x80043f24 */
};

#endif

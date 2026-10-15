/* The particle / effect "bit" family (BIT.H, BIT2.H, SMOKE.H, LIGHT.H, SHATTER.H, EXP.H): CBit and
   everything derived from it, plus their helper structs.

   egcs ABI: CBit is the root polymorphic class, so the vptr sits after its data at +0x44 (not
   declared here: the port build inserts it, egcs places it there); sizeof(CBit) == 0x48. Virtual
   slots (retail vtables, build/vtables.txt):
     slot 0  destructor
     slot 1  Move()     CBit::Move is an inline empty body (demo BIT.H:79; retail copy 0x8002ee64)
     slot 2  Display()  CSpecialDisplay and below only (inline empty, BIT.H:91; 0x8002ee5c)
   Every retail layout here equals the THPS2 demo's (MAIN.SYM); the census and the `new` sizes in
   the TUs (0x5c CSpark, 0x70 CMotionBlur, 0x74 CFlatRipple, 0x78 CGrindSpark/CRibbonBit, 0x9c
   CQuadBit) agree. See build/hdr_notes/bit.md. */
#ifndef GAME_BIT_H
#define GAME_BIT_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

/* CPPPSX doesn't define __cplusplus: wrap the SDK headers (SVECTOR) as the TUs do */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/classsizes.h"
#include "game/types.h"
#include "game/vector.h"

class CBody;
struct Texture;
struct SAnimFrame;
struct SLineSeg;

class CRibbonBit;
class CSmokeTrail;
class CWobblyGlow;

/* ---- helper structs ---------------------------------------------------------------------- */

/* one ring of a CGlow */
struct SSection {
	uint Radius;  /* 0x0 */
	uint CodeBGR; /* 0x4  (demo: PadBGR) */
	              /* sizeof == 0x8 */
};

/* one fringe quad of a CGlow */
struct SFringeQuad {
	uint Width;   /* 0x0 */
	uint CodeBGR; /* 0x4 */
	              /* sizeof == 0x8 */
};

/* one point of a CTexturedRibbon / CGouraudRibbon */
struct SRibbonPoint {
	CVector Pos; /* 0x00 */
	uchar r;     /* 0x0c */
	uchar g;     /* 0x0d */
	uchar b;     /* 0x0e */
	char pad0f[0x10 - 0x0f];
	ushort Width;  /* 0x10 */
	ushort WidthB; /* 0x12 */
	uchar rB;      /* 0x14 */
	uchar gB;      /* 0x15 */
	uchar bB;      /* 0x16 */
	char pad17[0x18 - 0x17];
	int Last1Scr; /* 0x18 */
	int Last2Scr; /* 0x1c */
	int Last3Scr; /* 0x20 */
	              /* sizeof == 0x24 */
};

/* CSmokeJet width wobble */
struct SWidthWobble {
	int TChange;     /* 0x0 */
	int TargetWidth; /* 0x4 */
	                 /* sizeof == 0x8 */
};

/* ---- CBit -------------------------------------------------------------------------------- */

/* base of every particle (BIT.CPP 0x80027d08..0x80027ec4); vtable 0x8009f8b0 */
class CBit {
  public:
	CBit *mPrevious;    /* 0x00 */
	CBit *mNext;        /* 0x04 */
	short mAge;         /* 0x08 */
	ushort mLifetime;   /* 0x0a  (CLight keeps its trigger node here) */
	CVector mTweenPos;  /* 0x0c */
	CVector mPos;       /* 0x18 */
	CVector mVel;       /* 0x24 */
	CVector mAcc;       /* 0x30 */
	CFriction mFric;    /* 0x3c */
	uchar mDead;        /* 0x3f */
	ushort mFrigDeltaZ; /* 0x40 */
	uchar mProtected;   /* 0x42 */
	uchar mType;        /* 0x43 */
	/* 0x44: vptr; sizeof(CBit) == 0x48 */

	static void *operator new(__SIZE_TYPE__ size) throw(); /* 0x80027d08 (zeroes the block) */
	void operator delete(void *);                 /* 0x80027d70 */
	void AttachTo(void *);                        /* 0x80027d90 */
	void DeleteFrom(void *);                      /* 0x80027dbc */
	void Die();                                   /* 0x80027e18 */
	void SetPos(const CVector &);                 /* 0x80027e24 */
	CBit();                                       /* 0x80027e44 */
	virtual ~CBit();                              /* slot 0: 0x80027e80 */
	virtual void Move() {}                        /* slot 1: 0x8002ee64 */

	/* inlined by retail into CSmokeGenerator::Move (bit2/move_smokegen.cpp) */
	void SetFrigDeltaZ(int v) {
		mFrigDeltaZ = v;
	}
};

/* vtable 0x8009f878 */
class CNonRenderedBit : public CBit {
  public:
	/* sizeof(CNonRenderedBit) == 0x48 */
	CNonRenderedBit();          /* 0x80027f64 */
	virtual ~CNonRenderedBit(); /* slot 0: 0x80027fa4 */
};

/* vtable 0x8009f890 */
class CSpecialDisplay : public CBit {
  public:
	/* sizeof(CSpecialDisplay) == 0x48 */
	CSpecialDisplay();          /* 0x80027ec4 */
	virtual ~CSpecialDisplay(); /* slot 0: 0x80027f04 */
	virtual void Display() {}   /* slot 2: 0x8002ee5c */
};

/* ---- flat textured sprites (POLY_FT4) ---------------------------------------------------- */

/* vtable 0x8009f860 */
class CFT4Bit : public CBit {
  public:
	short mTransDecay;      /* 0x48 */
	short mScaleDecay;      /* 0x4a */
	uint mCodeBGR;          /* 0x4c */
	SAnimFrame *mpPSXAnim;  /* 0x50 */
	SAnimFrame *mpPSXFrame; /* 0x54 */
	uchar mBitFlags;        /* 0x58 */
	uchar mNumFrames;       /* 0x59 */
	char mFrame;            /* 0x5a */
	uchar mFrameFrac;       /* 0x5b */
	short mAnimSpeed;       /* 0x5c */
	short mScale;           /* 0x5e */
	/* sizeof(CFT4Bit) == 0x60 */

	CFT4Bit();                         /* 0x80028004 */
	virtual ~CFT4Bit();                /* slot 0: 0x80028054 */
	void SetScale(ushort);             /* 0x800280a8 */
	void SetSemiTransparent();         /* 0x800280b0 */
	void SetTint(uchar, uchar, uchar); /* 0x800280c4 */
	void SetTransDecay(int);           /* 0x800280f8 */
	void SetAnim(char *);              /* 0x80028100 */
	void SetFrame(int);                /* 0x80028140 */
	void IncFrame();                   /* 0x80028160 */
	void IncFrameWithWrap();           /* 0x800281c0 */
	int Shrink(int);                   /* 0x80028234 */
	int Fade(int);                     /* 0x80028290 */
};

/* vtable 0x8009f7d0 */
class CFlatBit : public CFT4Bit {
  public:
	short mAngle;                /* 0x60 */
	short mAngVel;               /* 0x62 */
	short mAngAcc;               /* 0x64 */
	ushort mAngFric;             /* 0x66 */
	uint mPostScale;             /* 0x68 */
	uchar mForcedPriority;       /* 0x6c */
	uchar mSemiTransparencyRate; /* 0x6d */
	ushort mCLUT;                /* 0x6e */
	/* sizeof(CFlatBit) == 0x70 */

	CFlatBit();          /* 0x8002930c */
	virtual ~CFlatBit(); /* slot 0: 0x80029368 */

	/* inlined by retail into CSmokeGenerator::Move (bit2/move_smokegen.cpp) */
	void SetAngle(int a) {
		mAngle = a;
	}
};

/* vtable 0x8009f770 */
class CSimpleAnim : public CFlatBit {
  public:
	int mDie;      /* 0x70 */
	int mDieFrame; /* 0x74 */
	/* sizeof(CSimpleAnim) == 0x78 */

	virtual ~CSimpleAnim(); /* slot 0: 0x80029920 */
	virtual void Move();    /* slot 1: 0x80029974 */
};

/* vtable 0x8009f758 */
class CMotionBlur : public CFlatBit {
  public:
	/* sizeof(CMotionBlur) == 0x70 */
	CMotionBlur(const CVector &, CVector &, char *, int, int, int); /* 0x80029a0c */
	virtual ~CMotionBlur();                                         /* slot 0: 0x80029ad8 */
	virtual void Move();                                            /* slot 1: 0x80029b2c */
};

/* vtable 0x8009f740 */
class CMotionBlurImmortal : public CMotionBlur {
  public:
	/* sizeof(CMotionBlurImmortal) == 0x70 */
	virtual ~CMotionBlurImmortal(); /* slot 0: 0x80029b88 */
	virtual void Move();            /* slot 1: 0x80029bdc */
};

/* vtable 0x8009f728 */
class CFrag : public CFlatBit {
  public:
	/* sizeof(CFrag) == 0x70 */
	virtual ~CFrag();    /* slot 0: 0x80029c38 */
	virtual void Move(); /* slot 1: 0x80029c8c */
};

/* vtable 0x8009f7b8 */
class CLinked2EndedBit : public CFT4Bit {
  public:
	CVector mEndPosA; /* 0x60 */
	CVector mEndPosB; /* 0x6c */
	/* sizeof(CLinked2EndedBit) == 0x78 */

	CLinked2EndedBit();          /* 0x800293c8 */
	virtual ~CLinked2EndedBit(); /* slot 0: 0x80029408 */
};

/* one segment of a CRibbon; vtable 0x8009f6d0 */
class CRibbonBit : public CLinked2EndedBit {
  public:
	/* sizeof(CRibbonBit) == 0x78 */
	CRibbonBit();          /* 0x80029d80 */
	virtual ~CRibbonBit(); /* slot 0: 0x80029db4 */
	virtual void Move();   /* slot 1: 0x80029e08 */
};

/* ---- quads, chunks, glows, pixels -------------------------------------------------------- */

/* vtable 0x8009f848 (CBody::mpShadow is one) */
class CQuadBit : public CBit {
  public:
	CVector mPosB;      /* 0x48 */
	CVector mPosC;      /* 0x54 */
	CVector mPosD;      /* 0x60 */
	CVector mTweenPosB; /* 0x6c */
	CVector mTweenPosC; /* 0x78 */
	CVector mTweenPosD; /* 0x84 */
	uint mCodeBGR;      /* 0x90 */
	Texture *mpTexture; /* 0x94 */
	uchar mFlags;       /* 0x98 */
	char pad99[0x9c - 0x99];
	/* sizeof(CQuadBit) == 0x9c */

	CQuadBit();                                                        /* 0x80028394 */
	virtual ~CQuadBit();                                               /* slot 0: 0x800283e0 */
	void SetTexture(char *, int);                                      /* 0x80028440 */
	void FlagOn(uchar);                                                /* 0x80028484 */
	void SetSemiTransparent();                                         /* 0x80028498 */
	void SetTransparency(uchar);                                       /* 0x800284ac */
	void OrientUsing(const CVector &, const SVECTOR &, int, int, int); /* 0x800284d8 */
};

/* vtable 0x8009f830 */
class CGenPoly : public CBit {
  public:
	ushort mGouraud;    /* 0x48 */
	ushort mPad;        /* 0x4a */
	SVECTOR mPosA;      /* 0x4c */
	SVECTOR mPosB;      /* 0x54 */
	SVECTOR mPosC;      /* 0x5c */
	SVECTOR mPosD;      /* 0x64 */
	CVector mWorldPosA; /* 0x6c */
	CVector mWorldPosB; /* 0x78 */
	CVector mWorldPosC; /* 0x84 */
	CVector mWorldPosD; /* 0x90 */
	CSVector mAngles;   /* 0x9c */
	char pada2[0xa4 - 0xa2];
	uint mClutV0U0;  /* 0xa4 */
	uint mTPageV1U1; /* 0xa8 */
	ushort mV2U2;    /* 0xac */
	ushort mV3U3;    /* 0xae */
	uint mB0G0R0;    /* 0xb0 */
	uint mPadB1G1R1; /* 0xb4 */
	uint mPadB2G2R2; /* 0xb8 */
	uint mPadB3G3R3; /* 0xbc */
	uint mTexWin;    /* 0xc0 */
	/* sizeof(CGenPoly) == 0xc4 */

	virtual ~CGenPoly(); /* slot 0: 0x8002870c */
};

/* vtable 0x8009f818 */
class CChunkBit : public CBit {
  public:
	SVECTOR mPosA;      /* 0x48 */
	SVECTOR mPosB;      /* 0x50 */
	SVECTOR mPosC;      /* 0x58 */
	SVECTOR mPosD;      /* 0x60 */
	CVector mWorldPosA; /* 0x68 */
	CVector mWorldPosB; /* 0x74 */
	CVector mWorldPosC; /* 0x80 */
	CVector mWorldPosD; /* 0x8c */
	CSVector mAngles;   /* 0x98 */
	char pad9e[0xa0 - 0x9e];
	uint mClutV0U0;  /* 0xa0 */
	uint mTPageV1U1; /* 0xa4 */
	ushort mV2U2;    /* 0xa8 */
	ushort mV3U3;    /* 0xaa */
	uint mB0G0R0;    /* 0xac */
	uint mB1G1R1;    /* 0xb0 */
	uint mB2G2R2;    /* 0xb4 */
	uint mB3G3R3;    /* 0xb8 */
	uint mTexWin;    /* 0xbc */
	/* sizeof(CChunkBit) == 0xc0 */

	CChunkBit(const CSVector &, const CSVector &, const CSVector &);       /* 0x8002876c */
	virtual ~CChunkBit();                                                  /* slot 0: 0x80028968 */
	void SetRGB(uchar, uchar, uchar);                                      /* 0x800289c8 */
	void SetUVs(ushort, ushort, uchar, uchar, uchar, uchar, uchar, uchar); /* 0x80028b1c */
	void CalculateWorldCoords();                                           /* 0x80028b6c */
};

/* a falling shard of a shattered item (SHATTER.CPP); vtable 0x8009f280 */
class CShatterBit : public CChunkBit {
  public:
	CSVector mAngVel; /* 0xc0 */
	char padc6[0xc8 - 0xc6];
	CSmokeTrail *mpSmokeTrail; /* 0xc8 */
	/* sizeof(CShatterBit) == 0xcc */

	CShatterBit(const CSVector &, const CSVector &, const CSVector &, const CVector &,
	            int);             /* 0x80020b84 */
	virtual ~CShatterBit();       /* slot 0: 0x80020c90 */
	void SetPos(const CVector &); /* 0x80020d0c */
	virtual void Move();          /* slot 1: 0x80020e8c */
};

/* vtable 0x8009f800 */
class CGlow : public CBit {
  public:
	SSection *mpSections;   /* 0x48 */
	SFringeQuad *mpFringes; /* 0x4c */
	uint mNumSections;      /* 0x50 */
	uint mNumFringes;       /* 0x54 */
	uint mCentreCodeBGR;    /* 0x58 */
	short mStepAngle;       /* 0x5c */
	uchar mSkipTriangles;   /* 0x5e */
	char pad5f[0x60 - 0x5f];
	short mAngle; /* 0x60 */
	char pad62[0x64 - 0x62];
	uint mMask; /* 0x64 */
	/* sizeof(CGlow) == 0x68 */

	CGlow(uint, uint);                                                          /* 0x80028c48 */
	CGlow(const CVector &, int, int, uchar, uchar, uchar, uchar, uchar, uchar); /* 0x80028d54 */
	virtual ~CGlow();                             /* slot 0: 0x80028f28 */
	void SetRadius(int);                          /* 0x80028f94 */
	void SetFringeWidth(uint, uint);              /* 0x80028fd4 */
	void SetCentreRGB(uchar, uchar, uchar);       /* 0x8002901c */
	void SetRGB(uchar, uchar, uchar);             /* 0x80029048 */
	void SetFringeRGB(uint, uchar, uchar, uchar); /* 0x800290a4 */
	void SetRadialRGB(int, uchar, uchar, uchar);  /* 0x80029110 */
	virtual void Move() {}                        /* slot 1: 0x8002ee54 (BIT.H:418) */
};

/* vtable 0x8009f7e8 */
class CWobblyGlow : public CGlow {
  public:
	int mInc[8];      /* 0x68 */
	int mT[8];        /* 0x88 */
	int mAmplitude;   /* 0xa8 */
	int mInnerRadius; /* 0xac */
	/* sizeof(CWobblyGlow) == 0xb0 */

	CWobblyGlow(const CVector &, int, int, int, uchar, uchar, uchar, uchar, uchar,
	            uchar);       /* 0x80029188 */
	virtual ~CWobblyGlow() {} /* slot 0: 0x8002ee00 (BIT.H:436) */
	virtual void Move();      /* slot 1: 0x80029270 */
};

/* a POLY_F4/TILE pixel; vtable 0x8009f7a0 */
class CPixel : public CBit {
  public:
	uint tag;          /* 0x48 */
	uchar r0;          /* 0x4c */
	uchar g0;          /* 0x4d */
	uchar b0;          /* 0x4e */
	uchar code;        /* 0x4f */
	uint mWidthHeight; /* 0x50 */
	/* sizeof(CPixel) == 0x54 */

	CPixel();          /* 0x80029468 */
	virtual ~CPixel(); /* slot 0: 0x800294a8 */
};

/* vtable 0x8009f788 */
class CSpark : public CPixel {
  public:
	uchar mFadeR; /* 0x54 */
	uchar mFadeG; /* 0x55 */
	uchar mFadeB; /* 0x56 */
	char pad57[0x58 - 0x57];
	int mFloorY; /* 0x58 */
	/* sizeof(CSpark) == 0x5c */

	CSpark(CVector &, int, int, int); /* 0x80029684 */
	virtual ~CSpark();                /* slot 0: 0x800297bc */
	virtual void Move();              /* slot 1: 0x80029810 */
};

/* a falling glass triangle (built by Shatter_Glass); vtable 0x8009f688 */
class CGlassBit : public CBit {
  public:
	CVector mPosA;   /* 0x48 */
	CVector mPosB;   /* 0x54 */
	CVector mPosC;   /* 0x60 */
	int mGroundY;    /* 0x6c */
	uchar mDefaultR; /* 0x70 */
	uchar mDefaultG; /* 0x71 */
	uchar mDefaultB; /* 0x72 */
	uchar mR;        /* 0x73 */
	uchar mG;        /* 0x74 */
	uchar mB;        /* 0x75 */
	uchar mFadeRate; /* 0x76 */
	char pad77[0x78 - 0x77];
	/* sizeof(CGlassBit) == 0x78 */

	CGlassBit(const CVector &, const CVector &, int, uchar, uchar, uchar, int, int,
	          int);       /* 0x8002a534 */
	virtual ~CGlassBit(); /* slot 0: 0x8002a77c */
	virtual void Move();  /* slot 1: 0x8002a7dc */
};

/* ---- ribbons ----------------------------------------------------------------------------- */

/* a chain of CRibbonBits; vtable 0x8009f6b8 */
class CRibbon : public CNonRenderedBit {
  public:
	int mNumBits;       /* 0x48 */
	int mPointsPerBit;  /* 0x4c */
	int mNumPoints;     /* 0x50 */
	int mCurrent;       /* 0x54 */
	CVector *mPoints;   /* 0x58 */
	CRibbonBit **mBits; /* 0x5c */
	/* sizeof(CRibbon) == 0x60 */

	CRibbon(const CVector &, int, int, char *, char *, int, int); /* 0x80029e28 */
	virtual ~CRibbon();                                           /* slot 0: 0x8002a018 */
	void SetTint(uchar, uchar, uchar);                            /* 0x8002a0e8 */
	void SetScale(int);                                           /* 0x8002a178 */
	void SetPos(CVector &);                                       /* 0x8002a1e8 */
};

/* vtable 0x8009f6a0 */
class CSmokeTrail : public CRibbon {
  public:
	int mFadeAway; /* 0x60 */
	/* sizeof(CSmokeTrail) == 0x64 */

	CSmokeTrail(CVector &, int, int, int, int); /* 0x8002a2ec */
	virtual ~CSmokeTrail();                     /* slot 0: 0x8002a448 */
	virtual void Move();                        /* slot 1: 0x8002a49c */
};

/* vtable 0x8009f708 (build/vtables.txt calls it TextScreenElement's: see the notes) */
class CGouraudRibbon : public CSpecialDisplay {
  public:
	int mTrail;             /* 0x48 */
	int mNumPoints;         /* 0x4c */
	SRibbonPoint *mpPoints; /* 0x50 */
	/* sizeof(CGouraudRibbon) == 0x54 */

	virtual ~CGouraudRibbon(); /* slot 0: 0x8002da78 */
	virtual void Display();    /* slot 2: 0x8002dbf0 */
};

/* vtable 0x8009f6e8 */
class CTexturedRibbon : public CSpecialDisplay {
  public:
	uchar mTrail; /* 0x48 */
	char pad49[0x4c - 0x49];
	Texture *mpTex;     /* 0x4c */
	uchar mDontDisplay; /* 0x50 */
	char pad51[0x54 - 0x51];
	int mU;                 /* 0x54 */
	int mV;                 /* 0x58 */
	int mDv;                /* 0x5c */
	int mWibble;            /* 0x60 */
	int mNumPoints;         /* 0x64 */
	SRibbonPoint *mpPoints; /* 0x68 */
	/* sizeof(CTexturedRibbon) == 0x6c */

	CTexturedRibbon(int, int);  /* 0x8002e1e0 */
	virtual ~CTexturedRibbon(); /* slot 0: 0x8002e290 */
	void SetTexture(uint);      /* 0x8002e2f0 */
	virtual void Display();     /* slot 2: 0x8002e31c */
};

/* a steam/smoke jet (SMOKE.CPP); vtable 0x8009e2c0 */
class CSmokeJet : public CTexturedRibbon {
  public:
	SWidthWobble mWidthWobble[10]; /* 0x06c */
	int mOrgX[10];                 /* 0x0bc */
	int mPhase[10];                /* 0x0e4 */
	int mFreq[10];                 /* 0x10c */
	int mAmp[10];                  /* 0x134 */
	uchar mScrollSpeed;            /* 0x15c */
	uchar mDamageBaddies;          /* 0x15d */
	ushort mDamage;                /* 0x15e */
	ushort mDamageTimer;           /* 0x160 */
	CSVector mAngles;              /* 0x162 */
	int mLength;                   /* 0x168 */
	/* sizeof(CSmokeJet) == 0x16c */

	CSmokeJet(const CVector &, const CSVector &, int, ushort, int, int, int, int, uchar,
	          uint);              /* 0x80015820 */
	virtual ~CSmokeJet();         /* slot 0: 0x80015b84 */
	void SetPos(const CVector &); /* 0x80015bd8 */
	virtual void Move();          /* slot 1: 0x80015c98 */
};

/* ---- lines (BIT2.CPP) -------------------------------------------------------------------- */

/* vtable 0x8009f670 */
class CPolyLine : public CBit {
  public:
	uchar mForcedPriority; /* 0x48 */
	char pad49[0x4c - 0x49];
	int mNumSegs;    /* 0x4c */
	SLineSeg *mSegs; /* 0x50 */
	CVector mStart;  /* 0x54 */
	/* sizeof(CPolyLine) == 0x60 */

	CPolyLine(int);            /* 0x800270e0 (FUN_800270e0; demo __9CPolyLinei) */
	virtual ~CPolyLine();      /* slot 0: 0x800271a0 */
	void SetSemiTransparent(); /* 0x80027210 */
};

/* an expanding ring on water (EXP.CPP); vtable 0x800a1308 */
class CFlatRipple : public CPolyLine {
  public:
	uchar mR; /* 0x60 */
	uchar mG; /* 0x61 */
	uchar mB; /* 0x62 */
	char pad63[0x64 - 0x63];
	int mMaxRadius; /* 0x64 */
	int mRadius;    /* 0x68 */
	int mGrowRate;  /* 0x6c */
	int mAngle;     /* 0x70 */
	/* sizeof(CFlatRipple) == 0x74 */

	CFlatRipple(const CVector &, int, uchar, uchar, uchar, int, int); /* 0x8004ea54 */
	virtual ~CFlatRipple();                                           /* slot 0: 0x8004eba0 */
	virtual void Move();                                              /* slot 1: 0x8004ebf4 */
};

/* vtable 0x8009f658 */
class CGPolyLine : public CBit {
  public:
	int mNumSegs;          /* 0x48 */
	SLineSeg *mSegs;       /* 0x4c */
	CVector mStart;        /* 0x50 */
	uchar mStartR;         /* 0x5c */
	uchar mStartG;         /* 0x5d */
	uchar mStartB;         /* 0x5e */
	uchar mCode;           /* 0x5f */
	uchar mForcedPriority; /* 0x60 */
	char pad61[0x64 - 0x61];
	/* sizeof(CGPolyLine) == 0x64 */

	CGPolyLine(int);           /* 0x8002725c */
	virtual ~CGPolyLine();     /* slot 0: 0x80027334 */
	void SetSemiTransparent(); /* 0x800273a4 */
};

/* vtable 0x8009f628 */
class CGLine : public CBit {
  public:
	uint mCodeBGR0; /* 0x48 */
	uint mPadBGR1;  /* 0x4c */
	CVector mStart; /* 0x50 */
	CVector mEnd;   /* 0x5c */
	/* sizeof(CGLine) == 0x68 */

	CGLine();          /* 0x80027590 */
	virtual ~CGLine(); /* slot 0: 0x800275ec */
};

/* vtable 0x8009f610 */
class CGrindSpark : public CGLine {
  public:
	CVector mOldPos; /* 0x68 */
	CBody *mpParent; /* 0x74 */
	/* sizeof(CGrindSpark) == 0x78 */

	CGrindSpark(CBody *, CVector &, int, int, int); /* 0x80027650 */
	virtual ~CGrindSpark();                         /* slot 0: 0x800277b8 */
	virtual void Move();                            /* slot 1: 0x8002780c */
};

/* emits CMotionBlur smoke puffs; vtable 0x8009f640 */
class CSmokeGenerator : public CNonRenderedBit {
  public:
	int mPuffs; /* 0x48 */
	uchar mR;   /* 0x4c */
	uchar mG;   /* 0x4d */
	uchar mB;   /* 0x4e */
	char pad4f[0x50 - 0x4f];
	int mVBase;       /* 0x50 */
	int mVRandom;     /* 0x54 */
	int mScaleBase;   /* 0x58 */
	int mScaleRandom; /* 0x5c */
	/* sizeof(CSmokeGenerator) == 0x60 */

	virtual ~CSmokeGenerator(); /* slot 0: 0x800273fc */
	virtual void Move();        /* slot 1: 0x80027450 */
};

/* ---- lights (LIGHT.CPP) ------------------------------------------------------------------ */

/* a dynamic light (mType 2; mLifetime holds its trigger node); vtable 0x8009f0b0 */
class CLight : public CNonRenderedBit {
  public:
	int mInnerRadius;  /* 0x48 */
	int mFringeRadius; /* 0x4c */
	uchar mr0;         /* 0x50 */
	uchar mg0;         /* 0x51 */
	uchar mb0;         /* 0x52 */
	uchar mr1;         /* 0x53 */
	uchar mg1;         /* 0x54 */
	uchar mb1;         /* 0x55 */
	uchar mOn;         /* 0x56 */
	char pad57[0x58 - 0x57];
	int mAmp;            /* 0x58 */
	int mClipDist;       /* 0x5c */
	CWobblyGlow *mpGlow; /* 0x60 */
	/* sizeof(CLight) == 0x64 */

	CLight(const CVector &, int, int, int, int, int, uchar, uchar, uchar, uchar, uchar,
	       uchar);       /* 0x8001f614 */
	virtual ~CLight();   /* slot 0: 0x8001f718 */
	virtual void Move(); /* slot 1: 0x8001f794 */
};
CHECK_CLASS_SIZE(CLight, SIZEOF_CLIGHT);

/* functions */
void Bit_RemoveDeadBits();                          /* 0x80027c54 */
void Bit_SetSparkTrajectoryCone(const CSVector &p); /* 0x8002952c */
void Bit_SetSparkSize(uint);                        /* 0x80029648 */
void Bit_SetSparkRGB(uchar, uchar, uchar);          /* 0x8002965c */
void Bit_SetSparkFadeRGB(uchar, uchar, uchar);      /* 0x80029670 */
void Bit_Move();                                    /* 0x8002aa6c */

#endif

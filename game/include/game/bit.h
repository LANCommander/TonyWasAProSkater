/* BIT.H: CBit, the base of every particle and effect, and the bits BIT.CPP implements (sprites,
   quads, chunks, glows, sparks, ribbons). The other bits have their own module headers:
   bit2.h (lines, grind sparks, smoke generator), smoke.h, light.h, shatter.h and exp.h.

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
#include "game/camera.h"
class CGLine;

class CBody;
struct Texture;
struct SAnimFrame;

class CRibbonBit;
class CSmokeTrail;

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

/* ---- CBit -------------------------------------------------------------------------------- */

/* CBit::mType: the THPS2 source's BITTYPE_ consts (BIT.H, in order from 0; from its sk2.vtg tag
   database, as game/script.h's names). THPS1 sets only CLight's and CChunkBit's; every other bit
   keeps the zeroed block's BITTYPE_UNIDENTIFIED. */
enum EBitType : uchar {
	BITTYPE_UNIDENTIFIED = 0,
	BITTYPE_GENERICLASER = 1,
	BITTYPE_LIGHT = 2,             /* CLight (found on NonRenderedBitList by trigger node) */
	BITTYPE_SMARTBOMB = 3,
	BITTYPE_BADDYRAPIDLASER = 4,
	BITTYPE_TRIANGLEFRAGMENT = 5,
	BITTYPE_QUADFRAGMENT = 6,      /* CChunkBit */
	BITTYPE_SPRAY = 7,
};

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
	EBitType mType;     /* 0x43 */
	/* 0x44: vptr; sizeof(CBit) == 0x48 */

	static void *operator new(__SIZE_TYPE__ size) throw(); /* zeroes the block */
	void operator delete(void *p);
	void AttachTo(void *pList);
	void DeleteFrom(void *pList);
	void Die();
	void SetPos(const CVector &Pos);
	CBit();
	virtual ~CBit();
	virtual void Move() {}

	/* inlined by retail into CSmokeGenerator::Move (bit2/move_smokegen.cpp) */
	void SetFrigDeltaZ(int v) {
		mFrigDeltaZ = v;
	}
};

/* vtable 0x8009f878 */
class CNonRenderedBit : public CBit {
  public:
	/* sizeof(CNonRenderedBit) == 0x48 */
	CNonRenderedBit();
	virtual ~CNonRenderedBit();
};

/* vtable 0x8009f890 */
class CSpecialDisplay : public CBit {
  public:
	/* sizeof(CSpecialDisplay) == 0x48 */
	CSpecialDisplay();
	virtual ~CSpecialDisplay();
	virtual void Display() {}
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

	CFT4Bit();
	virtual ~CFT4Bit();
	void SetScale(ushort Scale);
	void SetSemiTransparent();
	void SetTint(uchar r, uchar g, uchar b);
	void SetTransDecay(int TransDecay);
	void SetAnim(char *pName);
	void SetFrame(int Frame);
	void IncFrame();
	void IncFrameWithWrap();
	int Shrink(int Kill);
	int Fade(int Kill);
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

	CFlatBit();
	virtual ~CFlatBit();

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

	virtual ~CSimpleAnim();
	virtual void Move();
};

/* vtable 0x8009f758 */
class CMotionBlur : public CFlatBit {
  public:
	/* sizeof(CMotionBlur) == 0x70 */
	CMotionBlur(const CVector &pos, CVector &vel, char *pAnimName, int scale, int scaleDecay,
	            int transDecay);
	virtual ~CMotionBlur();
	virtual void Move();
};

/* vtable 0x8009f740 */
class CMotionBlurImmortal : public CMotionBlur {
  public:
	/* sizeof(CMotionBlurImmortal) == 0x70 */
	virtual ~CMotionBlurImmortal();
	virtual void Move();
};

/* vtable 0x8009f728 */
class CFrag : public CFlatBit {
  public:
	/* sizeof(CFrag) == 0x70 */
	virtual ~CFrag();
	virtual void Move();
};

/* vtable 0x8009f7b8 */
class CLinked2EndedBit : public CFT4Bit {
  public:
	CVector mEndPosA; /* 0x60 */
	CVector mEndPosB; /* 0x6c */
	/* sizeof(CLinked2EndedBit) == 0x78 */

	CLinked2EndedBit();
	virtual ~CLinked2EndedBit();
};

/* one segment of a CRibbon; vtable 0x8009f6d0 */
class CRibbonBit : public CLinked2EndedBit {
  public:
	/* sizeof(CRibbonBit) == 0x78 */
	CRibbonBit();
	virtual ~CRibbonBit();
	virtual void Move();
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

	CQuadBit();
	virtual ~CQuadBit();
	void SetTexture(char *pName, int Frame);
	void FlagOn(uchar Flags);
	void SetSemiTransparent();
	void SetTransparency(uchar Level);
	void OrientUsing(const CVector &Position, const SVECTOR &Normal, int dx, int dy, int Angle);
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

	virtual ~CGenPoly();
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

	CChunkBit(const CSVector &a, const CSVector &b, const CSVector &c);
	virtual ~CChunkBit();
	void SetRGB(uchar r, uchar g, uchar b);
	void SetUVs(ushort clut, ushort tpage, uchar u0, uchar v0, uchar u1, uchar v1, uchar u2,
	            uchar v2);
	void CalculateWorldCoords();
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

	CGlow(uint NumPoints, uint NumFringes);
	CGlow(const CVector &Pos, int InnerRadius, int FringeRadius, uchar r0, uchar g0, uchar b0,
	      uchar r1, uchar g1, uchar b1);
	virtual ~CGlow();
	void SetRadius(int radius);
	void SetFringeWidth(uint fringe, uint width);
	void SetCentreRGB(uchar r, uchar g, uchar b);
	void SetRGB(uchar r, uchar g, uchar b);
	void SetFringeRGB(uint fringe, uchar r, uchar g, uchar b);
	void SetRadialRGB(int section, uchar r, uchar g, uchar b);
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

	CWobblyGlow(const CVector &Pos, int InnerRadius, int FringeRadius, int Wobble, uchar r0,
	            uchar g0, uchar b0, uchar r1, uchar g1, uchar b1);
	virtual ~CWobblyGlow() {} /* slot 0: 0x8002ee00 (BIT.H:436) */
	virtual void Move();
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

	CPixel();
	virtual ~CPixel();
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

	CSpark(CVector &pos, int vel, int gravity, int life);
	virtual ~CSpark();
	virtual void Move();
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

	CGlassBit(const CVector &Pos, const CVector &Vel, int GroundY, uchar r, uchar g, uchar b,
	          int RangeX, int RangeY, int RangeZ);
	virtual ~CGlassBit();
	virtual void Move();
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

	CRibbon(const CVector &pos, int numbits, int pointsperbit, char *pMiddleAnimName,
	        char *pEndAnimName, int scale, int semitrans);
	virtual ~CRibbon();
	void SetTint(uchar r, uchar g, uchar b);
	void SetScale(int scale);
	void SetPos(CVector &pos);
};

/* vtable 0x8009f6a0 */
class CSmokeTrail : public CRibbon {
  public:
	int mFadeAway; /* 0x60 */
	/* sizeof(CSmokeTrail) == 0x64 */

	CSmokeTrail(CVector &pos, int numbits, int r, int g, int b);
	virtual ~CSmokeTrail();
	virtual void Move();
};

/* vtable 0x8009f708 (build/vtables.txt calls it TextScreenElement's: see the notes) */
class CGouraudRibbon : public CSpecialDisplay {
  public:
	int mTrail;             /* 0x48 */
	int mNumPoints;         /* 0x4c */
	SRibbonPoint *mpPoints; /* 0x50 */
	/* sizeof(CGouraudRibbon) == 0x54 */

	virtual ~CGouraudRibbon();
	virtual void Display();
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

	CTexturedRibbon(int NumPoints, int Trail);
	virtual ~CTexturedRibbon();
	void SetTexture(unsigned int checksum);
	virtual void Display();
};

/* functions */
void Bit_Init();
void Bit_DeleteAll();
void Bit_RemoveDeadBits();
void Bit_SetSparkTrajectory(const CSVector &v);
void Bit_SetSparkTrajectoryCone(const CSVector &p);
void Bit_CalculateSparkVelocity(CVector &out, int mag);
void Bit_SetSparkSize(uint a);
void Bit_SetSparkRGB(uchar a, uchar b, uchar c);
void Bit_SetSparkFadeRGB(uchar a, uchar b, uchar c);
void Bit_Move();
void Bit_Display();

/* globals */
extern CGLine *GLineList;
extern SCamera MikeCamera[];
extern uchar *PolyBufferEnd;
/* the draw-mode packet the bits copy (native: no retail address) */
extern Texture *pShadowTexture;
extern SAnimFrame *QuickAnimLookup[];
extern int SparkSemiTrans;
#endif

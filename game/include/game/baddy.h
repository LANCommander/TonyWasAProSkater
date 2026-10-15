/* CBaddy (BADDY.H / BADDY.CPP): the script-driven world object (CSuper + a script interpreter),
   base of CPlatform (platform.h) and CCar (car.h); CScriptOnlyBaddy is the invisible, script-only
   kind (weather, smoke jets, looping sounds).

   Retail layout = the demo's (MAIN.SYM) shifted +8 (retail CSuper is 0x198, the demo's 0x190).
   CBaddy's vtable (0x800a0270) extends CBody's slots 0..6 with ExecuteCommand/SetVariable/
   GetVariable (slots 7..9). See build/hdr_notes/baddy.md. */
#ifndef GAME_BADDY_H
#define GAME_BADDY_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/vector.h"
#include "game/ob.h"

class CBruce;
class CSmokeJet;
struct SRainEntry;

class CBaddy : public CSuper {
  public:
	uint mMode;                    /* 0x198 */
	uchar mDoScript;               /* 0x19c */
	uchar mReadHooks;              /* 0x19d */
	uchar mApplyGravity;           /* 0x19e */
	uchar mHit;                    /* 0x19f */
	uchar mInputSignal;            /* 0x1a0 */
	uchar mDeathPulseSent;         /* 0x1a1 */
	uchar mComplexSurfaceInterval; /* 0x1a2 */
	uchar mComplexSurfaceTimer;    /* 0x1a3 */
	ushort mInputFlagsCopy;        /* 0x1a4 */
	ushort mSubType;               /* 0x1a6 */
	ushort mVerticalCompliance;    /* 0x1a8 */
	char pad1aa[0x1ac - 0x1aa];
	uint mSpecificFlags;        /* 0x1ac */
	ushort *mpScript;           /* 0x1b0 */
	ushort *mpScriptLabel[8];   /* 0x1b4 */
	ushort mBaddyAttributes[6]; /* 0x1d4 */
	ushort *mpWeaponAttributes; /* 0x1e0 */
	uchar mWeapons;             /* 0x1e4 */
	uchar mAttributesPerWeapon; /* 0x1e5 */
	uchar mCollisionOn;         /* 0x1e6 */
	char pad1e7[0x1e8 - 0x1e7];
	short mGroundToOrigin; /* 0x1e8 */
	short mOriginToEye;    /* 0x1ea */
	short mRegister[6];    /* 0x1ec */
	ushort mPattern;       /* 0x1f8 */
	ushort mSubPattern;    /* 0x1fa */
	ushort mWaitTimer;     /* 0x1fc */
	char pad1fe[0x200 - 0x1fe];
	uint mDeadChecksum;        /* 0x200 */
	CVector mTargetPos;        /* 0x204 */
	uint mLoopingSound;        /* 0x210 */
	int mLoopingSoundUpdate;   /* 0x214 */
	CBruce *mpCreatedByPlayer; /* 0x218 */
	/* sizeof(CBaddy) == 0x21c */

	CBaddy();
	virtual ~CBaddy();
	virtual void Die();
	void SendDeathPulse();
	void SkipToEndif();
	void ParseScript(ushort *p_script);
	virtual int ExecuteCommand(ushort command);
	int GetScriptValue();
	virtual void SetVariable(ushort variable);
	virtual short GetVariable(ushort variable);

	void StopScript() {
		mDoScript = 0;
	}
};

class CScriptOnlyBaddy : public CBaddy {
  public:
	char pad21c[0x220 - 0x21c];
	ushort mWeatherData; /* 0x220 */
	char pad222[0x224 - 0x222];
	SRainEntry *mpWeather;   /* 0x224 */
	CSmokeJet *mpSmokeJet;   /* 0x228 */
	uint mLoopingSound;      /* 0x22c (hides CBaddy's) */
	int mLoopingSoundUpdate; /* 0x230 (hides CBaddy's) */
	int mLoopingSoundVol;    /* 0x234 */
	/* sizeof(CScriptOnlyBaddy) == 0x238 */

	CScriptOnlyBaddy(short *p_info, int node);
	virtual ~CScriptOnlyBaddy();
	virtual void AI();
	virtual void SetVariable(ushort variable);
	virtual int ExecuteCommand(ushort command);
};

/* globals */
extern int CurrentWaterDamage;
extern int GameFMV_CurrentFrame;
#endif

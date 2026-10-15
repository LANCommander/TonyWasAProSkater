/* The skater camera (CAMERA.H / CAMERA.CPP): CCamera (a CBody), its CamPos positions, the
   AnimParam<T> template, the M3D camera struct SCamera (MikeCamera[]) and the camera presets.

   CCamera's retail layout is the demo's (MAIN.SYM) shifted by +0x2c, because CBody grew from 0x108
   to 0x134; nothing else moved. sizeof(CCamera) == 0x6b0 (demo 0x684).

   Member destruction order matters: ~CCamera (0x800788b8) destroys old_orientation, testQuat,
   mStoppedO..mO, mPosB..mPosA in reverse declaration order, then ~CBody; ~CamPos (0x8007d20c)
   destroys mWorldO, mLenseO, mStickO. So the CQuat/CamPos/AnimParam members are by value, in
   offset order. See build/hdr_notes/camera.md. */
#ifndef GAME_CAMERA_H
#define GAME_CAMERA_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

/* CPPPSX doesn't define __cplusplus: wrap the SDK headers ourselves (see ob.h) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/types.h"
#include "game/class.h"
#include "game/vector.h"
#include "game/ob.h"
#include "game/quat.h"

/* demo CAMERA.H enums */
enum ECameraMode {
	CAMERAMODE_NOTHING,         /* 0x00 */
	CAMERAMODE_NORMAL,          /* 0x01 */
	CAMERAMODE_NO_BIG_AIR,      /* 0x02 */
	CAMERAMODE_DEMO,            /* 0x03 */
	CAMERAMODE_START,           /* 0x04 */
	CAMERAMODE_FAR,             /* 0x05 */
	CAMERAMODE_OVERHEAD,        /* 0x06 */
	CAMERAMODE_FRONT,           /* 0x07 */
	CAMERAMODE_IDLE,            /* 0x08 */
	CAMERAMODE_FLYING,          /* 0x09 */
	CAMERAMODE_FUNKYFLYING,     /* 0x0a */
	CAMERAMODE_ROLLERCOASTER,   /* 0x0b */
	CAMERAMODE_PAN,             /* 0x0c */
	CAMERAMODE_ITSYLOOKDOWN,    /* 0x0d */
	CAMERAMODE_ITSYLOOKUP,      /* 0x0e */
	CAMERAMODE_LOOSE,           /* 0x0f */
	CAMERAMODE_USER,            /* 0x10 */
	CAMERAMODE_LOOKAROUND,      /* 0x11 */
	CAMERAMODE_UPSIDETEST,      /* 0x12 */
	CAMERAMODE_BOSSBEAST,       /* 0x13 */
	CAMERAMODE_BOSSWAR,         /* 0x14 */
	CAMERAMODE_BOSSTANK,        /* 0x15 */
	CAMERAMODE_DEBUG,           /* 0x16 */
	CAMERAMODE_COMPETITIONINTRO /* 0x17 */
};

enum ECameraModeIncDir { ASCENDING, DESCENDING };

enum EShakeType { CAMERASHAKE_BIG, CAMERASHAKE_MEDIUM, CAMERASHAKE_SMALL };

/* M3D's camera (M3D.H): MikeCamera[2] (one per player), passed to M3d_RenderSetup. Same layout as
   the demo. */
struct SCamera {
	uint Style;       /* 0x00 */
	VECTOR Position;  /* 0x04 */
	VECTOR Focus;     /* 0x14 */
	int Distance;     /* 0x24 */
	int Height;       /* 0x28 */
	SVECTOR Angles;   /* 0x2c */
	MATRIX Transform; /* 0x34 */
	MATRIX Inverse;   /* 0x54 */
	MATRIX View;      /* 0x74 */
	VECTOR Up;        /* 0x94 */
	                  /* sizeof(SCamera) == 0xa4 */
};

/* Presets[5] (Camera_Preset selects one) */
struct SPresetCamera {
	short YOffset; /* 0x0 */
	short Pitch;   /* 0x2 */
	short Dist;    /* 0x4 */
	char pad6[0x8 - 0x6];
	char *pName; /* 0x8 */
	             /* sizeof(SPresetCamera) == 0xc */
};

/* An animated parameter: points at a timer and holds the value. The constructor is inlined into
   CCamera's. The destructor is inline (demo CAMERA.H:141): ~CCamera (0x800788b8) inlines it,
   keeping its dead vptr store (an empty `{}` body does that; an implicit dtor doesn't). Retail's
   one out-of-line instance _._t9AnimParam1Z5CQuat is 0x8007d900, vtable 0x800a3594. */
template <class T> class AnimParam : public CClass {
  public:
	uint *mTimer; /* 0x04 */
	T mParam;     /* 0x08 */
	/* sizeof(AnimParam<CQuat>) == 0x1c */

	AnimParam(uint &timer) : mTimer(&timer) {}
	virtual ~AnimParam() {} /* slot 0 */
};

/* egcs: #pragma interface doesn't cover template instantiations, so without this every TU that
   constructs/destroys a CCamera would emit its own AnimParam<CQuat> dtor + vtable and stop
   matching (retail has them once, from CAMERA.CPP). The port lets g++ instantiate them. */

/* A camera position (vtable 0x800a35a4). The calc_/lock_ flags are bool in the demo (no type in
   MAIN.SYM; 4 bytes under egcs/MIPS): int here, as every TU declares them. */
class CamPos : public CClass {
  public:
	int calc_mTripod;     /* 0x04 */
	CBody *mTripod;       /* 0x08 */
	CVector mTripodPos;   /* 0x0c */
	int lock_mStickO;     /* 0x18 */
	CQuat mStickO;        /* 0x1c */
	CVector mStickOffset; /* 0x30 */
	short mStickLength;   /* 0x3c */
	char pad3e[0x40 - 0x3e];
	int calc_mLenseO;     /* 0x40 */
	CQuat mLenseO;        /* 0x44 */
	CVector mLenseOffset; /* 0x58 */
	int calc_mFocusPos;   /* 0x64 */
	CBody *mFocus;        /* 0x68 */
	CVector mFocusPos;    /* 0x6c */
	CQuat mWorldO;        /* 0x78 */
	CVector mWorldPos;    /* 0x8c */
	MATRIX mTrans;        /* 0x98 */
	/* sizeof(CamPos) == 0xb8 */

	CamPos();
	virtual ~CamPos();
	void Display(ushort x, ushort y, uchar r, uchar g, uchar b);
	static void DisplayLabels(ushort x, ushort y, uchar r, uchar g, uchar b);
};

/* The camera (vtable 0x800a3554: overrides slot 0 ~CCamera and slot 2 AI). The demo's bool members
   (mCalcTripod, new_focus, mCalcFocus, mOnRail, mStoppedOValid, mSpeedUpIntro, big_air) are int. */
class CCamera : public CBody {
  public:
	ushort mMikeIndex; /* 0x134 */
	char pad136[0x138 - 0x136];
	CamPos mPosA;                /* 0x138 */
	CamPos mLastPos;             /* 0x1f0 */
	CamPos mCurrentPos;          /* 0x2a8 */
	CamPos mPosB;                /* 0x360 */
	uint mCamPosDelta;           /* 0x418 */
	uint mCamPosRate;            /* 0x41c */
	CBody *mTripod;              /* 0x420 */
	CBody *mOldTripod;           /* 0x424 */
	int mCalcTripod;             /* 0x428 */
	CVector mOldTripodPos;       /* 0x42c */
	int new_focus;               /* 0x438 */
	CVector mTripodPos;          /* 0x43c */
	CVector mTripodPosIncrement; /* 0x448 */
	uint mTripodTime;            /* 0x454 */
	CBody *mFocus;               /* 0x458 */
	int mCalcFocus;              /* 0x45c */
	CVector mOldFocusPos;        /* 0x460 */
	CVector mFocusPos;           /* 0x46c */
	CVector mFocusPosIncrement;  /* 0x478 */
	uint mFocusTime;             /* 0x484 */
	int mZoom;                   /* 0x488 */
	ushort mZoomTime;            /* 0x48c */
	char pad48e[0x490 - 0x48e];
	int mZoomIncrement;      /* 0x490 */
	int mOnRail;             /* 0x494 */
	CVector mRailStart;      /* 0x498 */
	CVector mRailEnd;        /* 0x4a4 */
	short mPitchDamper;      /* 0x4b0 */
	short mTripodPosYDamper; /* 0x4b2 */
	CVector mUp;             /* 0x4b4 */
	CQuat mO;                /* 0x4c0 */
	CQuat mDesiredO;         /* 0x4d4 */
	CQuat mPrevO;            /* 0x4e8 */
	CQuat mPreFocusO;        /* 0x4fc */
	CQuat mStoppedO;         /* 0x510 */
	int mStoppedOValid;      /* 0x524 */
	CSVector mCamAngles;     /* 0x528 */
	char pad52e[0x530 - 0x52e];
	CVector mPosOffset;       /* 0x530 */
	CVector mDesiredPos;      /* 0x53c */
	CVector mDesiredFocusPos; /* 0x548 */
	int mCollisionRayLR;      /* 0x554 */
	int mCollisionRayBack;    /* 0x558 */
	short mCollisionAngLR;    /* 0x55c */
	short mCollisionAngBack;  /* 0x55e */
	short mCollisionBack;     /* 0x560 */
	char pad562[0x564 - 0x562];
	int saved_mCollisionRayLR;     /* 0x564 */
	int saved_mCollisionRayBack;   /* 0x568 */
	short saved_mCollisionAngLR;   /* 0x56c */
	short saved_mCollisionAngBack; /* 0x56e */
	short saved_mCollisionBack;    /* 0x570 */
	CSVector mShakeAmplitude;      /* 0x572 */
	CFriction mShakeDecay;         /* 0x578 */
	char pad57b[0x57c - 0x57b];
	CSVector mShakeSpeed; /* 0x57c */
	char pad582[0x584 - 0x582];
	ECameraMode mCameraMode;             /* 0x584 */
	ECameraModeIncDir mCameraModeIncDir; /* 0x588 */
	int mBossCamSpinRate;                /* 0x58c */
	uint mTimer;                         /* 0x590 */
	AnimParam<CQuat> testQuat;           /* 0x594 */
	short mColAngBuffer[8];              /* 0x5b0 */
	short mPrevColAngBuffer[8];          /* 0x5c0 */
	uchar mColCheckIndex;                /* 0x5d0 */
	char pad5d1[0x5d4 - 0x5d1];
	uint mColCheckCycleCounter; /* 0x5d4 */
	int mDownDist;              /* 0x5d8 */
	int mCompIntroTimer;        /* 0x5dc */
	int mSpeedUpIntro;          /* 0x5e0 */
	CVector mCompIntroEndPos;   /* 0x5e4 */
	int extra_YRot;             /* 0x5f0 */
	CVector mVFront;            /* 0x5f4 */
	CVector mOldVFront;         /* 0x600 */
	int prev_speed_lag_dist;    /* 0x60c */
	int big_air;                /* 0x610 */
	int big_air_timer;          /* 0x614 */
	int big_air_end_timer;      /* 0x618 */
	int big_air_mph;            /* 0x61c */
	int new_hang_time;          /* 0x620 */
	int in_pool_timer;          /* 0x624 */
	int jump_acc;               /* 0x628 */
	int jump_uptimer;           /* 0x62c */
	int max_jump_dist;          /* 0x630 */
	int max_jump_dist2;         /* 0x634 */
	int jump_timer;             /* 0x638 */
	int max_timer;              /* 0x63c */
	int landing_timer;          /* 0x640 */
	int landing_pos;            /* 0x644 */
	int turn_delay_timer;       /* 0x648 */
	CVector vFront;             /* 0x64c */
	uint mSpeed;                /* 0x658 */
	int mSpeedBuffer[6];        /* 0x65c */
	short mStoppedExtraYRot;    /* 0x674 */
	char pad676[0x678 - 0x676];
	VECTOR real_cam_offset; /* 0x678 */
	CQuat old_orientation;  /* 0x688 */
	VECTOR old_offset;      /* 0x69c */
	int loose_cam_timer;    /* 0x6ac */
	/* sizeof(CCamera) == 0x6b0 */

	CCamera(CBody *tripod, ushort mike_index);
	virtual ~CCamera();
	void SetCollisionRayLR(int v);
	void SetCollisionRayBack(int v);
	void SetCollisionAngLR(short v);
	void SetCollisionAngBack(short v);
	void Shake(const CVector &pos, EShakeType type);
	void SetComplexMode(ushort mode); /* empty */
	void LoadIntoMikeCamera();
	void ResetOrientation();
	void ReactToCollision();
	void MoveToDesiredPos(uint normal_slerp);
	void SetCamAngle(short angle, ushort speed); /* empty */
	void SetCamXZDistance(ushort dist, ushort speed); /* empty */
	void SetCamYDistance(short dist, ushort speed); /* empty */
	void SetCamXOffset(short offset, ushort speed); /* empty */
	void SetCamYOffset(short offset, ushort speed); /* empty */
	void SetCamZOffset(short offset, ushort speed); /* empty */
	void SetZoom(ushort zoom, ushort time);
	int GetZoom(); /* FUN_; demo GetZoom__7CCamera */
	void TweakMenu(); /* FUN_; demo TweakMenu__7CCamera */
	virtual void AI();
	void CM_Normal();
	void StartCompetitionIntro();
	void CM_CompetitionIntro();
	int InCompetitionIntro(); /* FUN_; demo returns bool */
	void CM_NoBigAir();

	/* inline (demo CAMERA.H; retail inlines them into CamPt_SetClosestCamera) */
	void SetMode(ECameraMode mode) {
		mCameraMode = mode;
	}
	/* inline in retail ExecuteCommandList (trig/executecommandlist.cpp) */
	void SetPitchDamper(short v) {
		mPitchDamper = v;
	}
	void SetTripodPosYDamper(short v) {
		mTripodPosYDamper = v;
	}
	void SetFocus(CBody *focus) {
		mCalcFocus = 1;
		mFocus = focus;
		new_focus = 1;
	}
	void SetTripod(CBody *tripod) {
		mCalcTripod = 1;
		mTripod = tripod;
	}
	void SetTripodPos(const CVector &pos) {
		mCalcTripod = 0;
		mTripodPos = pos;
		mTripod = 0;
		new_focus = 1;
	}
	/* inline in retail ReactToCollision (camera/reacttocollision.cpp) */
	int ColAng(int i) {
		return (mPrevColAngBuffer[i] + mColAngBuffer[i]) >> 1;
	}
};

void SendShake(const CVector &pos, CBody *b, EShakeType type);

/* functions */
class CBruce;


/* globals */
extern int camDist;
extern uint Camera_CurrentMikeCamera;
extern int CameraCollisionCheck;
extern int camPitch;
extern int camYOffset;
extern int Cheat_ScreenShots;
#endif

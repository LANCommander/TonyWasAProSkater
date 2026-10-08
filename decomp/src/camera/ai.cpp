/* AI__7CCamera (0x8007b7c4) flags=-fstructure-noalias */
/* C++ (CAMERA.CPP). Demo twin (opcode-identical but for 3 spots); layout by THPS1 offsets
   (CCamera members = demo + 0x2c). Pad is the SControl[2] array at Pad. */
#pragma interface

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

struct CVector {
	int vx, vy, vz;
};

struct CSVector {
	short vx, vy, vz;
};

struct CFriction {
	uchar x, y, z;
};

class CClass {
  public:
	virtual ~CClass() {}
};

class CQuat : public CClass {
  public:
	int x, y, z, w;
};

struct SButton {
	uchar Pressed;
	uchar Triggered;
	int PressedTime;
	int ReleasedTime;
	int TriggeredTime;
};

struct SControl {
	SButton Triangle, Square, Circle, X;
	SButton LeftOne, LeftTwo, RightOne, RightTwo;
	SButton Left, Right, Up, Down;
	SButton AnalogueLeft, AnalogueRight, Start, Select;
	char pad100[0x16c - 0x100];
};

class CBruce {
  public:
	char pad0[4];
	CVector mPos;
	char pad10[0x70 - 0x10];
	int mVelY; /* 0x70 */
	char pad74[0x358 - 0x74];
	int m358; /* 0x358 */
	char pad35c[0x734 - 0x35c];
	int m734; /* 0x734 */
	char pad738[0x8fe - 0x738];
	ushort m8fe; /* 0x8fe */
};

struct SViewport {
	char pad0[14];
	short mZoom;
};

class CCamera {
  public:
	char pad0[4];
	CVector mPos;     /* 0x004 */
	CSVector mAngles; /* 0x010 */
	char pad16[0x134 - 0x16];
	ushort mMikeIndex; /* 0x134 */
	char pad136[0x420 - 0x136];
	CBruce *mTripod; /* 0x420 */
	CBruce *mOldTripod;
	int mCalcTripod;       /* 0x428 */
	CVector mOldTripodPos; /* 0x42c */
	int new_focus;         /* 0x438 */
	CVector mTripodPos;    /* 0x43c */
	CVector mTripodPosIncrement;
	uint mTripodTime;
	CBruce *mFocus;       /* 0x458 */
	int mCalcFocus;       /* 0x45c */
	CVector mOldFocusPos; /* 0x460 */
	CVector mFocusPos;    /* 0x46c */
	CVector mFocusPosIncrement;
	uint mFocusTime;
	int mZoom;          /* 0x488 */
	ushort mZoomTime;   /* 0x48c */
	int mZoomIncrement; /* 0x490 */
	char pad494[0x4b0 - 0x494];
	short mPitchDamper;      /* 0x4b0 */
	short mTripodPosYDamper; /* 0x4b2 */
	char pad4b4[0x4c0 - 0x4b4];
	CQuat mO; /* 0x4c0 */
	char pad4d4[0x572 - 0x4d4];
	CSVector mShakeAmplitude; /* 0x572 */
	CFriction mShakeDecay;    /* 0x578 */
	CSVector mShakeSpeed;     /* 0x57c */
	int mCameraMode;          /* 0x584 */
	int mCameraModeIncDir;
	int mBossCamSpinRate;
	uint mTimer; /* 0x590 */
	char pad594[0x5f0 - 0x594];
	int extra_YRot; /* 0x5f0 */
	char pad5f4[0x628 - 0x5f4];
	int jump_acc;     /* 0x628 */
	int jump_uptimer; /* 0x62c */
	char pad630[0x658 - 0x630];
	uint mSpeed; /* 0x658 */
	void AI();
	void CM_Normal();
	void CM_NoBigAir();
	void CM_CompetitionIntro();
	void MoveToDesiredPos(uint);
	void LoadIntoMikeCamera();
};

struct SSinCos {
	short s, c;
};

extern SSinCos rcossin_tbl;

#define SIN(a) ((&rcossin_tbl)[(a) & 4095].s)

extern SControl Pad[2];
extern CVector Camera_CamOffset;
extern int Cheat_ScreenShots;
extern int GameFrozen;
extern int GamePaused;
extern SViewport *pCurrentViewport;
extern int TTime;

int Camera_CurrentMikeCamera;
int camDist;
int camPitch;
int camYOffset;
int Camera_CamPitch;
int Camera_CamDist;
int Mick_CameraTweak;
int DAT_800d2084;
int DAT_800d2088;
int DAT_800d208c;
int DAT_800d2090;
int old_camYOffset;
int old_camDist;
int old_camPitch;

CVector operator<<(const CVector &, const int &);
CVector operator+(const CVector &, const CVector &);
CVector operator/(const CVector &, const int &);

int operator==(const CVector &, const CVector &);

CQuat operator*(const CQuat &, const CQuat &);
CQuat Quat_FromXRot(short);
CQuat Quat_FromYRot(short);
CQuat Quat_FromZRot(short);
int Utils_CalcAim(CSVector &, const CVector &, const CVector &);

void CCamera::AI() {
	{
		int calc = mCalcTripod;

		Camera_CurrentMikeCamera = mMikeIndex;

		if (calc) {
			mOldTripodPos = mTripodPos;

			if (new_focus)
				mTripodPos = mTripod->mPos;
			else
				mTripodPos = ((mTripod->mPos << 1) + mOldTripodPos) / 3;
		}
	}

	if (mCalcFocus) {
		mOldFocusPos = mFocusPos;

		if (new_focus)
			mFocusPos = mFocus->mPos;
		else
			mFocusPos = ((mFocus->mPos << 1) + mOldFocusPos) / 3;
	}

	if (Pad[0].Select.Pressed) {
		if (DAT_800d2090)
			DAT_800d208c = 1;

		DAT_800d2090 = 0;
	} else {
		DAT_800d2090 = 1;
	}

	if (Cheat_ScreenShots && DAT_800d208c && Pad[0].Select.Pressed) {
		Pad[1].RightOne.Triggered = 0;
		Pad[1].RightTwo.Triggered = 0;
		DAT_800d2084 = 0;
		Pad[0].LeftTwo.Triggered = 0;
		DAT_800d208c = 0;
		Pad[0].Select.Triggered = 0;
		Pad[0].Start.Triggered = 0;
		Mick_CameraTweak ^= 1;
		GameFrozen = Mick_CameraTweak;

		if (Mick_CameraTweak) {
			int y = camYOffset;
			int d = camDist;
			int p = camPitch;
			int z = mZoom;

			old_camYOffset = y;
			old_camDist = d;
			old_camPitch = p;
			DAT_800d2088 = z;
		}
	}

	if (Mick_CameraTweak) {
		if (Pad[1].Start.Pressed) {
			camYOffset = old_camYOffset;
			camDist = old_camDist;
			camPitch = old_camPitch;
			mZoom = DAT_800d2088;
		}

		if (Pad[0].Square.Pressed)
			mZoom--;

		if (Pad[0].Circle.Pressed)
			mZoom++;

		if (Pad[0].Triangle.Pressed)
			mZoom = 256;

		if (Pad[1].Square.Pressed)
			extra_YRot -= 10;

		if (Pad[1].Circle.Pressed)
			extra_YRot += 10;

		if (Pad[1].Left.Pressed) {
			if (Pad[1].Select.Pressed)
				camDist -= 32;
			else
				camDist -= 8;
		}

		if (Pad[1].Right.Pressed) {
			if (Pad[1].Select.Pressed)
				camDist += 32;
			else
				camDist += 8;
		}

		if (Pad[1].Up.Pressed) {
			if (Pad[1].Select.Pressed)
				camPitch -= 32;
			else
				camPitch -= 8;
		}

		if (Pad[1].Down.Pressed) {
			if (Pad[1].Select.Pressed)
				camPitch += 32;
			else
				camPitch += 8;
		}

		camPitch &= 4095;

		if (Pad[1].LeftOne.Pressed) {
			if (Pad[1].Select.Pressed)
				camYOffset -= 32;
			else
				camYOffset -= 8;
		}

		if (Pad[1].LeftTwo.Pressed) {
			if (Pad[1].Select.Pressed)
				camYOffset += 32;
			else
				camYOffset += 8;
		}
	}

	{
		int dist = camDist;
		int pitch = camPitch;
		int yoff = camYOffset;

		Camera_CamOffset.vx = 0;
		Camera_CamOffset.vz = 0;
		Camera_CamDist = dist;
		Camera_CamPitch = pitch;
		Camera_CamOffset.vy = yoff << 12;
	}

	if (GamePaused) {
		if (Mick_CameraTweak) {
			CM_Normal();
			MoveToDesiredPos(311);
		}
	} else {
		mTimer++;

		if (mZoomTime) {
			mZoomTime--;
			mZoom += mZoomIncrement;
		}

		pCurrentViewport->mZoom = mZoom;
		Utils_CalcAim(mAngles, mPos, mFocusPos);

		if (mTripod->m8fe && mTripodPosYDamper) {
			if (mTripod->mVelY < 0)
				jump_uptimer++;
			else
				jump_uptimer--;

			jump_acc += (-mTripod->mVelY >> 12) * mTripodPosYDamper;

			if (jump_uptimer > 0)
				mTripodPos.vy += jump_acc;
		} else {
			jump_acc = 0;
			jump_uptimer = 0;
		}

		short damp = mPitchDamper;

		if (damp) {
			int vy = mTripod->mVelY;

			if (vy < 0)
				Camera_CamPitch -= damp;

			if (vy > 0)
				Camera_CamPitch -= damp >> 1;

			Camera_CamPitch &= 4095;
		}

		switch (mCameraMode) {
			case 1:
				CM_Normal();
				break;
			case 2:
				CM_NoBigAir();
				break;
			case 23:
				CM_CompetitionIntro();
				goto Done;
		}

		if (mTripod->m358)
			MoveToDesiredPos(311);
		else if (mTripodPos == mOldTripodPos)
			MoveToDesiredPos(151);
		else if (mSpeed < 4)
			MoveToDesiredPos(151);
		else if (mTripod->m734 == 1) {
			if (mTimer < 18)
				MoveToDesiredPos(311);
			else
				MoveToDesiredPos(151);
		} else
			MoveToDesiredPos(311);

		Utils_CalcAim(mAngles, mPos, mFocusPos);

		if (mShakeAmplitude.vx || *(int *)&mShakeAmplitude.vy) {
			mO = Quat_FromYRot(mShakeAmplitude.vy * SIN(mShakeSpeed.vy * TTime) / 4096) *
			     mO *
			     Quat_FromXRot(mShakeAmplitude.vx * SIN(mShakeSpeed.vx * TTime) / 4096) *
			     Quat_FromZRot(mShakeAmplitude.vz * SIN(mShakeSpeed.vz * TTime) / 4096);

			{
				CSVector oldamp = mShakeAmplitude;

				if (mShakeAmplitude.vx < 0)
					mShakeAmplitude.vx += mShakeDecay.x;
				else
					mShakeAmplitude.vx -= mShakeDecay.x;

				if (mShakeAmplitude.vy < 0)
					mShakeAmplitude.vy += mShakeDecay.y;
				else
					mShakeAmplitude.vy -= mShakeDecay.y;

				if (mShakeAmplitude.vz < 0)
					mShakeAmplitude.vz += mShakeDecay.z;
				else
					mShakeAmplitude.vz -= mShakeDecay.z;

				if (((ushort)oldamp.vx ^ (ushort)mShakeAmplitude.vx) & 0x8000)
					mShakeAmplitude.vx = 0;

				if (((ushort)oldamp.vy ^ (ushort)mShakeAmplitude.vy) & 0x8000)
					mShakeAmplitude.vy = 0;

				if (((ushort)oldamp.vz ^ (ushort)mShakeAmplitude.vz) & 0x8000)
					mShakeAmplitude.vz = 0;
			}
		}
	Done:
		LoadIntoMikeCamera();
	}
}

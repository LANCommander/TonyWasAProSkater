/* DoInAirPhysics__6CBruce (0x8003609c) */
/* C++ (PHYSICS.CPP). Demo twin, locals from its MAIN.SYM. In the air: moves the skater by the
   frame-scaled velocity and kick, applies the surface spin, turns by the spin input, keeps the
   skater upright, then sweeps the movement against the world. Hitting a wall that is steep
   enough at a shallow angle starts a wall ride (or bails when going too slowly for the wall);
   otherwise a wall knocks the skater back down and a floor lands them (grind / manual / bail
   checks). No hit: the gravity is added to the kick.
   Levers: unused CVector locals keep retail's frame slots; WallRight is declared just before its
   GTE use (so the by-value SVECTOR argument temps share its slot, as in retail); the unused
   SquareRoot0 call goes through a void cast and the libgte calls return MATRIX * / VECTOR *
   (v0/v1 choice); the m368 trick-limit test is if / else-if with a Trick index local; the
   landing sound is a switch with a call per arm; m200 = 15 is stored twice around the mPos
   update (the demo's MoveOriginTo call sat between the two stores). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

struct CVector {
	int vx, vy, vz;
	CVector &operator+=(const CVector &);
	int operator==(const CVector &);
};

CVector operator*(const CVector &, const int &);
CVector operator>>(const CVector &, const int &);
CVector operator/(const CVector &, const int &);
CVector operator+(const CVector &, const CVector &);
CVector operator-(const CVector &, const CVector &);

#define SetVector(v, x, y, z)                                                                      \
	{                                                                                              \
		int _x = (x);                                                                              \
		int _y = (y);                                                                              \
		int _z = (z);                                                                              \
		(v)->vx = _x;                                                                              \
		(v)->vy = _y;                                                                              \
		(v)->vz = _z;                                                                              \
	}

struct SItem;

struct SFace {
	char pad0[0xe];
	unsigned short Flags; /* 0xe */
};

struct SLineInfo {
	CVector StartCoords;
	CVector EndCoords;
	CVector MinCoords;
	CVector MaxCoords;
	int iLo, iHi, jLo, jHi;
	int Distance; /* 0x40 */
	int Length;
	char pad48[0x68 - 0x48];
	SItem *pItem;     /* 0x68 */
	CVector HitPoint; /* 0x6c */
	SVECTOR Normal;   /* 0x78 */
	SFace *pFace;     /* 0x80 */
	char pad84[168 - 0x84];
};

struct SColl {
	unsigned char b0; /* 0x0 */
	char pad1[8 - 1];
	int x8; /* 0x8 */
	char padc[0xa0 - 0xc];
	unsigned char bA0; /* 0xa0 */
};

struct STrickInfo {
	char pad0[0x18];
	int f18; /* 0x18 */
	char pad1c[44 - 0x1c];
};

class CBruce {
  public:
	char pad0[0x4];
	CVector mPos; /* 0x4 */
	char pad10[0x6c - 0x10];
	CVector mVel;  /* 0x6c */
	CVector mKick; /* 0x78 */
	char pad84[0xa8 - 0x84];
	SVECTOR mLastNormal; /* 0xa8 */
	char padb0[0xf0 - 0xb0];
	CVector mOldPos; /* 0xf0 */
	char padfc[0x134 - 0xfc];
	int m134; /* 0x134 */
	char pad138[0x144 - 0x138];
	short mFrame; /* 0x144 */
	char pad146[0x14e - 0x146];
	unsigned char m14E; /* 0x14e */
	char pad14f[0x150 - 0x14f];
	int m150; /* 0x150 */
	char pad154[0x1c8 - 0x154];
	CVector mGravity; /* 0x1c8 */
	char pad1d4[0x1f8 - 0x1d4];
	int m1F8; /* 0x1f8 */
	char pad1fc[0x200 - 0x1fc];
	int m200; /* 0x200 */
	char pad204[0x208 - 0x204];
	int m208; /* 0x208 */
	char pad20c[0x220 - 0x20c];
	int m220; /* 0x220 */
	char pad224[0x288 - 0x224];
	int m288; /* 0x288 */
	char pad28c[0x2fc - 0x28c];
	int m2FC; /* 0x2fc */
	char pad300[0x328 - 0x300];
	int m328; /* 0x328 */
	int m32C; /* 0x32c */
	char pad330[0x358 - 0x330];
	int mInAir; /* 0x358 */
	int m35C;   /* 0x35c */
	int m360;   /* 0x360 */
	char pad364[0x368 - 0x364];
	int m368; /* 0x368 */
	char pad36c[0x72c - 0x36c];
	int m72C; /* 0x72c */
	char pad730[0x734 - 0x730];
	int m734; /* 0x734 */
	char pad738[0x740 - 0x738];
	int m740; /* 0x740 */
	char pad744[0x764 - 0x744];
	int m764; /* 0x764 */
	int m768; /* 0x768 */
	char pad76c[0x78c - 0x76c];
	MATRIX mRotMatrix; /* 0x78c */
	char pad7ac[0x7d8 - 0x7ac];
	CVector mFront; /* 0x7d8 */
	CVector mRight; /* 0x7e4 */
	CVector mUp;    /* 0x7f0 */
	char pad7fc[0x814 - 0x7fc];
	CVector m814; /* 0x814 */
	int m820;     /* 0x820 */
	CVector m824; /* 0x824 */
	char pad830[0x834 - 0x830];
	int m834; /* 0x834 */
	int m838; /* 0x838 */
	char pad83c[0x86c - 0x83c];
	SColl *mpColl; /* 0x86c */
	char pad870[0x895 - 0x870];
	signed char m895; /* 0x895 */
	void StateTransition(int, int);
	void OrientToNormal();
	void RotMatrixFromFRUVectors();
	void DoInAirPhysics();
};

extern int XFrames;
extern int TimeScale;
extern int TimeScaleSquared;
extern int GLevel;
extern int BulletCollisionCheck;
extern CVector ZeroVector;
extern CVector WorldUp;
extern STrickInfo Group[];
extern char s_New_Bail[];

int Face_Air;
int Face_Skateable;
int Face_WallRide;
int Face_Visible;
int Face_Terrain;

void Platform_HandleBounce();
void ProjectCVectorToPlane(CVector *, SVECTOR);
void M3dUtils_NormaliseVector(CVector *);
void M3dColij_InitLineInfo(SLineInfo *);
int GetFaceFlags(SLineInfo *);
int SFX_PlayX(unsigned int, short, int);
int Rnd(int);

extern "C" {
	int Mips_CVecDotProd__FP7CVectorT0(void *, void *);
	int FUN_8008b5dc(int);
	int Mips_FixMul__Fii(int, int);
	int M3dZone_LineToItemNotSCached__FP9SLineInfoi(SLineInfo *, int);
	MATRIX *FUN_80093100(MATRIX *, VECTOR *);
	MATRIX *FUN_80093240(SVECTOR *, MATRIX *);
	MATRIX *FUN_80093130(MATRIX *, MATRIX *);
	VECTOR *FUN_80092f60(MATRIX *, VECTOR *, VECTOR *);
	void UprightSideways__6CBruce(CBruce *);
	void Bail__6CBrucei13ELimitBailVel(CBruce *, int);
	void FUN_8003036c(CBruce *);
	void FlipToVelocity__6CBruce(CBruce *);
	void FUN_8002f1a0(CBruce *);
	void Debug_RunAnim__6CBruceii(CBruce *, int, int);
	void ReflectHeadingVelocityInPlane__6CBruceG7SVECTORi(CBruce *, SVECTOR, int);
	void HandleBailPerpendicular__6CBruceG7SVECTOR(CBruce *, SVECTOR);
	void FUN_80035f30(CBruce *, SVECTOR);
	void HandleBailOnLanding__6CBrucei(CBruce *, int);
	int printf(const char *, ...);
}

void CBruce::DoInAirPhysics() {
	CVector u0, u1, u2; /* unused: retail's frame keeps their slots */
	CVector vMovement;
	CVector u3, u4;
	int Speed;
	int TurnAngle;
	MATRIX SurfaceRotMatrix;
	SVECTOR RotAngle;
	SLineInfo line_info;
	CVector Gravity;

	Gravity = mGravity;
	mPos += ((mVel * TimeScale) >> 8) + ((mKick * TimeScaleSquared) >> 8) / 2;
	Platform_HandleBounce();

	if (m834) {
		VECTOR zero, v, v2, v3;

		zero.vz = 0;
		zero.vy = 0;
		zero.vx = 0;
		FUN_80093100(&mRotMatrix, &zero);
		FUN_80093100(&SurfaceRotMatrix, &zero);

		v.vx = 0;
		v.vy = 0x46000;
		v.vz = 0;
		FUN_80092f60(&mRotMatrix, &v, &v2);

		RotAngle.vz = 0;
		RotAngle.vy = 0;
		RotAngle.vx = m834 & 4095;
		FUN_80093240(&RotAngle, &SurfaceRotMatrix);
		FUN_80093130(&mRotMatrix, &SurfaceRotMatrix);
		FUN_80092f60(&mRotMatrix, &v, &v3);

		mPos.vx = mPos.vx + v2.vx - v3.vx;
		mPos.vy = mPos.vy + v2.vy - v3.vy;
		mPos.vz = mPos.vz + v2.vz - v3.vz;
	}

	vMovement = mPos - mOldPos;
	((void (*)(int))FUN_8008b5dc)(Mips_CVecDotProd__FP7CVectorT0(&vMovement, &vMovement));
	TurnAngle = m838 >> 12;
	Mips_FixMul__Fii(TurnAngle, 3072);

	if (TurnAngle && m814 == ZeroVector) {
		RotAngle.vz = 0;
		RotAngle.vx = 0;
		RotAngle.vy = TurnAngle & 4095;
		FUN_80093240(&RotAngle, &SurfaceRotMatrix);
		FUN_80093130(&mRotMatrix, &SurfaceRotMatrix);

		m328 += TurnAngle;

		if (m328 >= 2048) {
			m328 -= 2048;

			if (!m288)
				m32C++;
		}

		if (m328 <= -2048) {
			m328 += 2048;

			if (!m288)
				m32C++;
		}
	}

	if (m734 != 2)
		UprightSideways__6CBruce(this);

	SetVector(&line_info.StartCoords, mOldPos.vx, mOldPos.vy, mOldPos.vz)
	    SetVector(&line_info.EndCoords, mPos.vx, mPos.vy, mPos.vz) vMovement = mPos - mOldPos;
	Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&vMovement, &vMovement)) << 6;

	if (Speed < 0x2e000) {
		if (Speed < 2048)
			Speed = 2048;

		line_info.EndCoords.vx = mOldPos.vx + ((vMovement.vx * 46 / Speed) << 12);
		line_info.EndCoords.vy = mOldPos.vy + ((vMovement.vy * 46 / Speed) << 12);
		line_info.EndCoords.vz = mOldPos.vz + ((vMovement.vz * 46 / Speed) << 12);
	}

	M3dColij_InitLineInfo(&line_info);
	M3dZone_LineToItemNotSCached__FP9SLineInfoi(&line_info, 1);
	GetFaceFlags(&line_info);

	if (Face_Air && !Face_Visible && m734 != 3) {
		BulletCollisionCheck = 1;
		M3dColij_InitLineInfo(&line_info);
		M3dZone_LineToItemNotSCached__FP9SLineInfoi(&line_info, 1);
		GetFaceFlags(&line_info);

		BulletCollisionCheck = 0;
	}

	if (line_info.pItem) {
		if ((!Face_Skateable || (Face_Air && Face_Visible)) && Face_WallRide && !mInAir &&
		    (mpColl->b0 || mpColl->x8 < 20)) {
			CVector UnitVel;
			CVector FaceNormal;
			int VelNormal;
			int Steep = line_info.Normal.vy >= 3278;

			UnitVel = mVel;
			UnitVel.vy = 0;
			M3dUtils_NormaliseVector(&UnitVel);

			FaceNormal.vx = line_info.Normal.vx;
			FaceNormal.vy = 0;
			FaceNormal.vz = line_info.Normal.vz;
			M3dUtils_NormaliseVector(&FaceNormal);

			VelNormal = Mips_CVecDotProd__FP7CVectorT0(&FaceNormal, &UnitVel);

			if (!Steep && ((VelNormal ^ (VelNormal >> 31)) - (VelNormal >> 31)) < 3547 &&
			    XFrames - m208 >= 41) {
				int Limit = 10000;
				int Trick = m368 & 255;

				if (m368 & 0x4000)
					Limit = m14E - Group[Trick].f18;
				else if (m368 & 0x2000)
					Limit = m14E - Group[Trick].f18;

				if (m35C && !m360 && mFrame < Limit) {
					Bail__6CBrucei13ELimitBailVel(this, 0);

					m220++;
				} else
					m360 = 1;

				if (!mInAir) {
					FUN_8003036c(this);
					StateTransition(5, 5197);

					m208 = XFrames;
					mPos.vx = line_info.HitPoint.vx + line_info.Normal.vx * 30;
					mPos.vy = line_info.HitPoint.vy + line_info.Normal.vy * 30;
					mPos.vz = line_info.HitPoint.vz + line_info.Normal.vz * 30;
					ProjectCVectorToPlane(&mFront, line_info.Normal);
					M3dUtils_NormaliseVector(&mFront);
					SetVector(&mUp, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
					    gte_ldopv1(&mFront);
					gte_ldopv2(&mUp);
					gte_op12();
					gte_stlvnl(&mRight);
					RotMatrixFromFRUVectors();
					FlipToVelocity__6CBruce(this);

					SetVector(&mLastNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz)
					    SetVector(&m824, line_info.Normal.vx, line_info.Normal.vy,
						          line_info.Normal.vz) m820 = 0;
					ProjectCVectorToPlane(&mVel, line_info.Normal);
					CVector WallRight;

					gte_ldopv1(&WorldUp);
					gte_ldopv2(&mUp);
					gte_op12();
					gte_stlvnl(&WallRight);
					RotMatrixFromFRUVectors();

					m150 = 0x10000;

					if (m134 & 2) {
						if (Mips_CVecDotProd__FP7CVectorT0(&WallRight, &mVel) >= 0)
							Debug_RunAnim__6CBruceii(this, 35, 5238);
						else
							Debug_RunAnim__6CBruceii(this, 36, 5242);
					} else {
						if (Mips_CVecDotProd__FP7CVectorT0(&WallRight, &mVel) >= 0)
							Debug_RunAnim__6CBruceii(this, 36, 5249);
						else
							Debug_RunAnim__6CBruceii(this, 35, 5253);
					}
				} else {
					mPos = mOldPos;
				}
			} else {
				mPos = mOldPos;
				printf(s_New_Bail);
				Bail__6CBrucei13ELimitBailVel(this, 0);
			}
		} else if (!Face_Skateable && (!Face_Air || m734 != 3)) {
			switch (Face_Terrain) {
				case 7:
					SFX_PlayX(53, 0x2000, 0);
					break;
				case 9:
					if (GLevel == 8)
						SFX_PlayX(137, 0x2000, 0);
					else
						SFX_PlayX(40, 0x2000, 0);

					break;
			}

			mLastNormal.vx = 0;
			mLastNormal.vy = -4096;
			mLastNormal.vz = 0;
			SetVector(&m824, 0, -4096, 0) m820 = 0;
			OrientToNormal();

			m200 = 15;
			mPos.vx = line_info.HitPoint.vx + line_info.Normal.vx;
			mPos.vy = line_info.HitPoint.vy + line_info.Normal.vy;
			mPos.vz = line_info.HitPoint.vz + line_info.Normal.vz;
			m200 = 15;
			StateTransition(1, 5363);

			m740 = 1;
			ReflectHeadingVelocityInPlane__6CBruceG7SVECTORi(this, line_info.Normal, 0);

			mVel.vx += line_info.Normal.vx / 4;
			mVel.vy += line_info.Normal.vy / 4;
			mVel.vz += line_info.Normal.vz / 4;

			if (!Rnd(3)) {
				mVel.vx += Rnd(16384) - 8192;
				mVel.vz += Rnd(16384) - 8192;
			}
		} else {
			mPos.vx = line_info.HitPoint.vx + line_info.Normal.vx * 30;
			mPos.vy = line_info.HitPoint.vy + line_info.Normal.vy * 30;
			mPos.vz = line_info.HitPoint.vz + line_info.Normal.vz * 30;

			if (!mInAir && Face_Air) {
				if (m764 && (!m1F8 || XFrames - m1F8 < 5 || Face_Visible)) {
					StateTransition(2, 5428);
				} else {
					StateTransition(1, 5435);

					if (mpColl->bA0 || m895 < -40) {
						SVECTOR N;
						int Spd = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

						N = line_info.Normal;
						mVel.vx -= Mips_FixMul__Fii(N.vx, Spd / 4);
						mVel.vz -= Mips_FixMul__Fii(N.vz, Spd / 4);
					}

					m768 = 0;

					if (!m740 || !m35C) {
						FUN_8002f1a0(this);

						m150 = 0x10000;
						Debug_RunAnim__6CBruceii(this, 14, 5457);

						m740 = 1;
					}
				}
			} else if (m734 != 3 || XFrames - m2FC >= 31) {
				StateTransition(0, 5487);

				m72C = Face_Terrain;

				if (!mInAir) {
					HandleBailPerpendicular__6CBruceG7SVECTOR(this, line_info.Normal);
					FUN_80035f30(this, line_info.Normal);
					HandleBailOnLanding__6CBrucei(this, 5496);
				}
			}

			m768 = line_info.pFace->Flags & 0x20;

			if (m734 != 1) {
				ProjectCVectorToPlane(&mVel, line_info.Normal);

				SetVector(&mLastNormal, line_info.Normal.vx, line_info.Normal.vy,
				          line_info.Normal.vz)
				    SetVector(&m824, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
				        m820 = 0;
				OrientToNormal();
			}
		}
	} else {
		mKick += Gravity;
	}
}

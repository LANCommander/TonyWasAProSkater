/* Vector math types (VECTOR.H / QUAT.H): CVector (20.12 fixed point int vector), CSVector (short
   vector, usually 12-bit angles), CFriction (per-axis friction shifts) and CQuat. Plain Vector /
   SVector are the M3D modules' own POD twins of CVector / CSVector.

   Member functions out of line live in VECTOR.CPP (0x8004d900..0x8004e1d4) and QUAT.CPP; the
   inline ones here are the bodies retail inlined (see build/hdr_notes/vector.md). */
#ifndef GAME_VECTOR_H
#define GAME_VECTOR_H

/* egcs: the inline members and vtables of this header's classes are not emitted in every TU that
   uses them (byte-exact TUs hold one function each); the port build drops the pragma */
#ifndef GAME_DATA_TU /* (the data files: full debug types, for data_image.py) */
#pragma interface
#endif

#include "game/types.h"
#include "game/class.h"

extern "C" int SquareRoot0(int); /* libgte; as <libgte.h> declares it */

struct CFriction {
	uchar vx; /* 0x0 */
	uchar vy; /* 0x1 */
	uchar vz; /* 0x2 */

	/* no default value: retail's CCamera ctor leaves mShakeDecay alone. CBit/CBody construct theirs
	   with CFriction(1, 1, 1) in their initialiser lists */
	CFriction() {}
	CFriction(uchar x, uchar y, uchar z) {
		vx = x;
		vy = y;
		vz = z;
	}
	void Set(uchar a, uchar b, uchar c) {
		vx = a;
		vy = b;
		vz = c;
	}
};

struct CVector {
	int vx; /* 0x0 */
	int vy; /* 0x4 */
	int vz; /* 0x8 */

	CVector() {}
	CVector(int x, int y, int z) {
		vx = x;
		vy = y;
		vz = z;
	}
	void Set(int x, int y, int z) {
		vx = x;
		vy = y;
		vz = z;
	}
	void Zero() {
		vx = vy = vz = 0;
	}

	int Length() const;                     /* 0x8004d900 */
	void Normalize();                       /* 0x8004d980 */
	void KillSmall();                       /* 0x8004d9e0 */
	CVector &operator+=(const CVector &);   /* 0x8004da3c */
	CVector &operator-=(const CVector &);   /* 0x8004da78 */
	CVector &operator*=(const int &);       /* 0x8004dab4 */
	CVector &operator/=(const int &);       /* 0x8004db04 */
	CVector &operator>>=(const int &);      /* 0x8004db54 */
	CVector &operator<<=(const int &);      /* 0x8004db90 */
	CVector &operator%=(const CFriction &); /* 0x8004dbcc */
	int operator==(const CVector &);        /* 0x8004dc18 */
	int operator!=(const CVector &);        /* 0x8004dc5c */
	CVector operator-() const;              /* 0x8004df2c */
};

CVector operator+(const CVector &, const CVector &); /* 0x8004dca4 */
CVector operator-(const CVector &, const CVector &); /* 0x8004dcfc */
CVector operator*(const CVector &, const int &);     /* 0x8004dd54 */
CVector operator*(const int &, const CVector &);     /* 0x8004ddbc */
CVector operator/(const CVector &, const int &);     /* 0x8004de24 */
CVector operator>>(const CVector &, const int &);    /* 0x8004de8c */
CVector operator<<(const CVector &, const int &);    /* 0x8004dedc */

struct CSVector {
	short vx; /* 0x0 */
	short vy; /* 0x2 */
	short vz; /* 0x4 */

	CSVector() {}
	CSVector(short x, short y, short z) {
		vx = x;
		vy = y;
		vz = z;
	}
	void Set(short x, short y, short z) {
		vx = x;
		vy = y;
		vz = z;
	}

	void Mask();                             /* 0x8004df58 */
	void KillSmall();                        /* 0x8004df80 */
	CSVector &operator+=(const CSVector &);  /* 0x8004dfe8 */
	CSVector &operator/=(const int &);       /* 0x8004e024 */
	CSVector &operator%=(const CFriction &); /* 0x8004e074 */
};

CSVector operator-(const CSVector &, const CSVector &); /* 0x8004e0c4 */
CSVector operator*(const int &, const CSVector &);      /* 0x8004e11c */
CSVector operator>>(const CSVector &, const int &);     /* 0x8004e184 */

/* CQuat: vptr at +0 (CClass), x/y/z/w 20.12 fixed point. Retail's vtable 0x800a3474 has one slot,
   ~CQuat (inline, QUAT.H:132), which TUs emit locally (0x80078088, 0x8007d944). */
class CQuat : public CClass {
  public:
	int x; /* 0x04 */
	int y; /* 0x08 */
	int z; /* 0x0c */
	int w; /* 0x10 */

	CQuat() { /* identity (CAMERA's static initialiser 0x8007d6e4) */
		x = 0;
		y = 0;
		z = 0;
		w = 4096;
	}
	CQuat(int a, int b, int c, int d) {
		x = a;
		y = b;
		z = c;
		w = d;
	}
	/* slot 0: the implicit (synthesized) ~CQuat: retail's copies store only the inlined ~CClass's
	   vptr, never CQuat's own (an explicit `{}` body would add that store) */

	CQuat &operator*=(const CQuat &q) {
		int tempw = (w * q.w - x * q.x - y * q.y - z * q.z) >> 12;
		int tempx = (w * q.x + x * q.w + y * q.z - z * q.y) >> 12;
		int tempy = (w * q.y + y * q.w + z * q.x - x * q.z) >> 12;
		int tempz = (w * q.z + z * q.w + x * q.y - y * q.x) >> 12;

		x = tempx;
		y = tempy;
		z = tempz;
		w = tempw;
		return *this;
	}
	void Normalise() {
		int mag = SquareRoot0(x * x + y * y + z * z + w * w);

		if (!mag)
			mag = 1;

		x = (x << 12) / mag;
		y = (y << 12) / mag;
		z = (z << 12) / mag;
		w = (w << 12) / mag;
	}
};

CQuat operator*(const CQuat &, const CQuat &); /* 0x8007722c */

/* M3D's plain vector structs (no member functions) */
struct Vector {
	int vx; /* 0x0 */
	int vy; /* 0x4 */
	int vz; /* 0x8 */
};

struct SVector {
	short vx; /* 0x0 */
	short vy; /* 0x2 */
	short vz; /* 0x4 */
};

/* functions */
int Mips_FixMul(int, int); /* 0x80072090 */
int Mips_CVecDotProd(CVector *, CVector *); /* 0x800720ec */

#endif

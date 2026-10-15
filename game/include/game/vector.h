/* VECTOR.H: the vector math types: CVector (20.12 fixed point int vector), CSVector (short vector,
   usually 12-bit angles) and CFriction (per-axis friction shifts); CQuat is in quat.h. Plain
   Vector / SVector are the M3D modules' own POD twins of CVector / CSVector.

   Member functions out of line live in VECTOR.CPP (0x8004d900..0x8004e1d4); the inline ones here
   are the bodies retail inlined (see build/hdr_notes/vector.md). */
#ifndef GAME_VECTOR_H
#define GAME_VECTOR_H

/* egcs: the inline members and vtables of this header's classes are not emitted in every TU that
   uses them (byte-exact TUs hold one function each); the port build drops the pragma */
#ifndef GAME_DATA_TU /* (the data files: full debug types) */
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

	int Length() const;
	void Normalize();
	void KillSmall();
	CVector &operator+=(const CVector &);   /* 0x8004da3c */
	CVector &operator-=(const CVector &);   /* 0x8004da78 */
	CVector &operator*=(const int &);       /* 0x8004dab4 */
	CVector &operator/=(const int &);       /* 0x8004db04 */
	CVector &operator>>=(const int &);      /* 0x8004db54 */
	CVector &operator<<=(const int &);      /* 0x8004db90 */
	CVector &operator%=(const CFriction &); /* 0x8004dbcc */
	int operator==(const CVector &);        /* 0x8004dc18 */
	int operator!=(const CVector &);        /* 0x8004dc5c */
	CVector operator-() const;
};

CVector operator+(const CVector &a, const CVector &b);
CVector operator-(const CVector &a, const CVector &b);
CVector operator*(const CVector &a, const int &n);
CVector operator*(const int &n, const CVector &a);
CVector operator/(const CVector &a, const int &n);
CVector operator>>(const CVector &a, const int &n);
CVector operator<<(const CVector &a, const int &n);

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

	void Mask();
	void KillSmall();
	CSVector &operator+=(const CSVector &);  /* 0x8004dfe8 */
	CSVector &operator/=(const int &);       /* 0x8004e024 */
	CSVector &operator%=(const CFriction &); /* 0x8004e074 */
};

CSVector operator-(const CSVector &a, const CSVector &b);
CSVector operator*(const int &n, const CSVector &a);
CSVector operator>>(const CSVector &a, const int &n);


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
int Mips_FixMul(int, int);
int Mips_CVecDotProd(CVector *, CVector *);

#endif

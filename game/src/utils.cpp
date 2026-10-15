/* UTILS.CPP: odds and ends - the vblank handler and Pause, vector helpers (distances, rotation,
   aim), string helpers, line of sight and ground height, killing and hiding objects in a box, and
   the spatial attenuation of sounds (declarations in game/utils.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgpu.h>
	#include <libgte.h>
}
#include "pc_draw.h"
#include "pc_frame.h"
#include "game/utils.h"
#include "game/debug.h"
#include "game/trig.h"
#include "game/colij.h"
#include "game/ob.h"
#include "game/bit.h"
#include "game/bruce.h"
#include "game/m3d.h"
#include "game/m3dasm.h"
#include "game/vector.h"
#include "game/camera.h"
#include "game/light.h"
#include "game/db.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/m3dzone.h"
#include "game/main.h"
#include "game/psyq.h"
#include "game/sfx.h"
#include "game/spool.h"

extern int DoVblankProcessing;
void Utils_VblankProcessing(void);
#define rsin(a)                                                                                    \
	({                                                                                             \
		SSinCos *p_ = &rcossin_tbl[(a) & 4095];                                                    \
		p_->s;                                                                                     \
	})
#define rcos(a)                                                                                    \
	({                                                                                             \
		SSinCos *p_ = &rcossin_tbl[(a) & 4095];                                                    \
		p_->c;                                                                                     \
	})
int FUN_80093604(int);
extern int DAT_800d1a18;
#define INSIDE(v, w, u)  (v >= t8 && t5 >= v && w >= t6 && t3 >= w && u >= t7 && t4 >= u)
#define OUTSIDE(v, w, u) ((v < t8 || t5 < v) && (w < t6 || t3 < w) && (u < t7 || t4 < u))
#define SET(o)           o->mFlags = (o->mFlags & t2) | t1

/* Rnd's state */
extern int RandSeed, RandMul, RandAdd;

/* Debug_String__FPciT0 (0x80047774) */
void Debug_String(char *, int, char *) {}

/* Utils_VblankProcessing__Fv (0x8004777c) */
void Utils_VblankProcessing(void) {
	if (GameFade) {
		POLY_F4 p;
		DR_TPAGE m;

		setlen(&m, 1);
		m.code[0] = 0xe1000640;
		setlen(&p, 5);
		GameFade--;
		p.code = 0x2a;

		if (GameFade & 1) {
			p.r0 = 0;
			p.g0 = 0;
			p.b0 = 8;
		} else {
			p.r0 = 8;
			p.g0 = 8;
			p.b0 = 0;
		}

		p.x0 = 0;
		p.x1 = 512;
		p.y0 = 0;
		p.y1 = 0;
		p.x2 = 0;
		p.x3 = 512;
		p.y2 = 240;
		p.y3 = 240;
		DrawSync(0);
		PC_DrawPacket(&m);
		PC_DrawPacket(&p);
		DrawSync(0);
	}

	if (SFX_ReverbDepthSetTimer) {
		if (--SFX_ReverbDepthSetTimer == 0)
			SFX_SetReverbDepth(32, 32);
	}
}

/* MyVSync__Fv (0x80047870) */
void MyVSync(void) {
	Vblanks++;
	Db_Vblank();
	SFX_ProcessingPerFrame();

	if (!GamePaused && !GameFrozen)
		Xblanks++;

	if (DoVblankProcessing)
		Utils_VblankProcessing();
}

/* Pause__Fi (0x800478f4) */
void Pause(int n) {
	unsigned t = *(volatile unsigned *)&Vblanks + n;

	/* PC: no interrupt counts the frames while this spins: wait for them (pc_frame.h) */
	while (*(volatile unsigned *)&Vblanks < t)
		PC_FrameWait(1);
}

/* Utils_InitialRand__Fi (0x80047928) */
/* Seeds the random-number generator. */
void Utils_InitialRand(int seed) {
	RandSeed = seed;
	RandMul = 0x12b9b0a1;
	RandAdd = 0x0aa2fb3f;
}

/* Rnd__Fi (0x8004794c) */
/* A random number in [0, n). */
int Rnd(int n) {
	int r;

	RandSeed = RandSeed * RandMul + RandAdd;
	r = (int)((RandSeed & 0xffff) * n);
	RandMul = (RandMul ^ RandSeed) + (RandSeed >> 4);
	RandAdd -= 0x10101010;
	RandAdd += RandSeed >> 3;
	return r >> 16;
}

/* Utils_RotateY__FR7CVectorRC7CVectori (0x800479a8) */
void Utils_RotateY(CVector &out, const CVector &in, int a) {
	SSinCos *b;
	SSinCos *t;

	a &= 4095;
	b = rcossin_tbl;
	t = b + a;
	out.vx = ((in.vx >> 3) * t->c + (in.vz >> 3) * t->s) >> 9;
	out.vy = in.vy;
	out.vz = ((in.vz >> 3) * t->c - (in.vx >> 3) * t->s) >> 9;
}

static inline void VI(CVector *p, int _x, int _y, int _z) {
	p->vx = _x;
	p->vy = _y;
	p->vz = _z;
}

/* Utils_CalcUnit__FR7CVectorRC7CVector (0x80047a34) */
/* (C++ compiles geom_square's `r = geom_sqr(...)` with an extra copy of the result, as in
   CVector::Length: the one difference from the C source's code) */
void Utils_CalcUnit(CVector &Unit, const CVector &Vec) {
	CVector u;
	int l;

	VI(&u, Vec.vx >> 12, Vec.vy >> 12, Vec.vz >> 12);
	geom_square(&u, &u);

	l = SquareRoot0(u.vx + u.vy + u.vz);

	if (l) {
		Unit.vx = Vec.vx / l;
		Unit.vy = Vec.vy / l;
		Unit.vz = Vec.vz / l;
	} else {
		Unit.vx = 0;
		Unit.vy = 0;
		Unit.vz = 0;
	}
}

/* Utils_GetVecFromMagDir__FR7CVectoriRC8CSVector (0x80047b24) */
/* (the angles read as unsigned, as the C source did) */
void Utils_GetVecFromMagDir(CVector &out, int mag, const CSVector &dir) {
	out.vx = -(((mag * rcos((ushort)dir.vx)) >> 12) * rsin((ushort)dir.vy));
	out.vy = mag * rsin((ushort)dir.vx);
	out.vz = -(((mag * rcos((ushort)dir.vx)) >> 12) * rcos((ushort)dir.vy));
}

/* Utils_Dist__FRC7CVectorT0 (0x80047bf4) */
int Utils_Dist(const CVector &a, const CVector &b) {
	CVector d;

	d.vx = (a.vx - b.vx) >> 12;
	d.vy = (a.vy - b.vy) >> 12;
	d.vz = (a.vz - b.vz) >> 12;
	return M3dMaths_SquareRoot0(d.vx * d.vx + d.vy * d.vy + d.vz * d.vz);
}

/* Utils_XZDist__FRC7CVectorT0 (0x80047c70) */
int Utils_XZDist(const CVector &a, const CVector &b) {
	int dx = (a.vx - b.vx) >> 12;
	int dz = (a.vz - b.vz) >> 12;

	return M3dMaths_SquareRoot0(dx * dx + dz * dz);
}

/* Utils_CrapDist__FRC7CVectorT0 (0x80047cc4) */
int Utils_CrapDist(const CVector &a, const CVector &b) {
	CVector d;

	d.vx = a.vx - b.vx;
	d.vy = a.vy - b.vy;
	d.vz = a.vz - b.vz;

	if (d.vx < 0)
		d.vx = -d.vx;

	if (d.vy < 0)
		d.vy = -d.vy;

	if (d.vz < 0)
		d.vz = -d.vz;

	if (d.vx < d.vy) {
		if (d.vy < d.vz)
			return ((d.vx >> 2) + (d.vy >> 1) + d.vz) >> 12;
		else if (d.vz < d.vx)
			return ((d.vz >> 2) + (d.vx >> 1) + d.vy) >> 12;
		else
			return ((d.vx >> 2) + (d.vz >> 1) + d.vy) >> 12;
	} else {
		if (d.vx < d.vz)
			return ((d.vy >> 2) + (d.vx >> 1) + d.vz) >> 12;
		else if (d.vz < d.vy)
			return ((d.vz >> 2) + (d.vy >> 1) + d.vx) >> 12;
		else
			return ((d.vy >> 2) + (d.vz >> 1) + d.vx) >> 12;
	}
}

/* Utils_CalcAim__FR8CSVectorRC7CVectorT1 (0x80047ddc) */
int Utils_CalcAim(CSVector &o, const CVector &a, const CVector &b) {
	int dx = (b.vx - a.vx) >> 12;
	int dy = (b.vy - a.vy) >> 12;
	int dz = (b.vz - a.vz) >> 12;
	int d;

	if (dz != 0) {
		if (dz > 0)
			o.vy = 2048 - FUN_80093604((dx << 12) / -dz);
		else
			o.vy = FUN_80093604((dx << 12) / dz);
	} else {
		o.vy = dx <= 0 ? 1024 : -1024;
	}

	d = M3dMaths_SquareRoot0(dx * dx + dz * dz);

	if (d != 0) {
		if (dy > 0)
			o.vx = FUN_80093604((unsigned)(dy << 12) / (unsigned)d);
		else
			o.vx = -FUN_80093604((unsigned)(-dy << 12) / (unsigned)d);
	} else {
		o.vx = dy <= 0 ? -1024 : 1024;
	}

	o.vz = 0;
	o.vx &= 4095;
	o.vy &= 4095;
	return d;
}

/* Utils_TurnTowards__FG8CSVectorR8CSVectorT1T0i (0x80047f40) */
void Utils_TurnTowards(CSVector a, CSVector &o1, CSVector &o2, CSVector b, int speed) {
	CSVector d;

	d.vx = b.vx - a.vx;
	d.vy = b.vy - a.vy;

	if (d.vx < -2048)
		d.vx += 4096;

	if (d.vx > 2048)
		d.vx -= 4096;

	if (d.vy < -2048)
		d.vy += 4096;

	if (d.vy > 2048)
		d.vy -= 4096;

	if (d.vx == 0 && d.vy == 0) {
		o1.vx = 0;
		o1.vy = 0;
	} else {
		o2.vx = (speed * d.vx) >> 8;
		o2.vy = (speed * d.vy) >> 8;
	}
}

/* Utils_ClearExplosionStack__Fv (0x80048044) */
void Utils_ClearExplosionStack(void) {
	DAT_800d1a18 = 0;
}

/* Utils_CompareStrings__FPCcT0Ui (0x80048050) */
int Utils_CompareStrings(const char *a, const char *b, unsigned n) {
	unsigned char c1, c2;

	if (!a && !b)
		return 1;

	if (!a || !b)
		return 0;

	do {
		c1 = *a++;
		c2 = *b++;

		if (c1 >= 65 && c1 <= 90)
			c1 += 32;

		if (c2 >= 65 && c2 <= 90)
			c2 += 32;
	} while (c1 == c2 && c1 && --n);

	return c1 == c2;
}

/* Utils_CopyString__FPCcPc (0x800480d4) */
void Utils_CopyString(const char *s, char *d) {
	do {
		*d++ = *s++;
	} while (s[-1]);
}

/* Utils_ArcCos__Fi (0x800480f8) */
int Utils_ArcCos(int a) {
	int flag;
	int d;
	int t;

	flag = 0;

	if (a < 0) {
		a = -a;
		flag = 1;
	}

	d = 256;
	t = 512;

	for (;;) {
		SSinCos *p = &rcossin_tbl[t];
		int c = p->c;

		if (a < c) {
			t += d;
		} else if (c == a)
			break;
		else
			t -= d;

		d >>= 1;

		if (d == 0)
			break;
	}

	if (flag)
		return 2048 - t;

	return t;
}

/* Utils_LineOfSight__FRC7CVectorT0 (0x80048170) */
int Utils_LineOfSight(const CVector &a, const CVector &b) {
	SLineInfo li;

	li.StartCoords.vx = a.vx;
	li.StartCoords.vy = a.vy;
	li.StartCoords.vz = a.vz;
	li.EndCoords.vx = b.vx;
	li.EndCoords.vy = b.vy;
	li.EndCoords.vz = b.vz;
	LineOfSightCheck = 1;
	M3dColij_InitLineInfo(&li);
	M3dZone_LineToItemNotSCached(&li, 1);

	LineOfSightCheck = 0;
	return li.pItem == 0;
}

/* Utils_GetGroundHeight__FRC7CVectorii (0x800481e8) */
int Utils_GetGroundHeight(const CVector &p, int up, int down) {
	SLineInfo li;

	li.StartCoords.vx = p.vx;
	li.StartCoords.vy = p.vy - (up << 12);
	li.StartCoords.vz = p.vz;
	li.EndCoords.vx = p.vx;
	li.EndCoords.vy = p.vy + (down << 12);
	li.EndCoords.vz = p.vz;
	M3dColij_InitLineInfo(&li);

	li.RecordTriggerZoneHits = 0;
	M3dZone_LineToItemNotSCached(&li, 1);

	if (li.pItem && li.Normal.vy < -3071)
		return li.Position.vy;

	return -1;
}

/* Utils_KillObjectsInBox__FRC7CVectorT0P5CBodyb (0x80048278) */
/* Demo twin; locals from its MAIN.SYM. Kills every live body in the list whose
   position lies inside the box: Die() or a straight delete. Returns how many went. */
int Utils_KillObjectsInBox(const CVector &min, const CVector &max, CBody *p_list, bool use_die) {
	int killed = 0;

	while (p_list) {
		CBody *p_next = (CBody *)p_list->mNextItem;

		if (!p_list->IsDead()) {
			CVector pos = p_list->mPos;

			if (pos.vx >= min.vx && max.vx >= pos.vx && pos.vz >= min.vz && max.vz >= pos.vz &&
			    pos.vy >= min.vy && max.vy >= pos.vy) {
				if (use_die)
					p_list->Die();
				else
					delete p_list;

				killed++;
			}
		}

		p_list = p_next;
	}

	return killed;
}

/* Utils_KillEverythingInBox__FRC7CVectorT0 (0x800483e8) */
int Utils_KillEverythingInBox(const CVector &a, const CVector &b) {
	int n = 0;

	n += Utils_KillObjectsInBox(a, b, BaddyList, 1);
	n += Utils_KillObjectsInBox(a, b, EnvironmentalObjectList, 1);
	n += Utils_KillObjectsInBox(a, b, PowerUpList, 1);
	n += Utils_KillObjectsInBox(a, b, SuspendedList, false);
	return n;
}

/* Utils_SetVisibilityInBox__FRC7CVectorT0bT2 (0x80048484) */
void Utils_SetVisibilityInBox(const CVector &a, const CVector &b, bool arg2, bool arg3) {
	int t8 = a.vx, t7 = a.vy, t6 = a.vz, t5 = b.vx, t4 = b.vy, t3 = b.vz;
	int t1, t2;
	CItem *o;
	CBit *q;

	if (pCurrentPlayer == 0 || GNumberOfPlayers == 1 || GGame == 7)
		t1 = 0x41;
	else {
		t1 = 1;

		if (pCurrentPlayer == MechList)
			t1 = 0x40;
	}

	t2 = ~t1;

	if (arg2)
		t1 = 0;
	else
		t2 = -1;

	o = (CItem *)EnviroList;

	if (o) {
		do {
			SPSXRegion *e = &PSXRegion[o->mRegion];

			if (e->Usable != 0) {
				int x = o->mPos.vx, y = o->mPos.vy, z = o->mPos.vz;

				if (arg3) {
					if (INSIDE(x, z, y))
						SET(o);
				} else {
					if (OUTSIDE(x, z, y))
						SET(o);
				}
			}

			o = o->mNextItem;
		} while (o);
	}

	o = PowerUpList;

	if (o) {
		do {
			if (arg3) {
				if (INSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			} else {
				if (OUTSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			}

			o = o->mNextItem;
		} while (o);
	}

	o = SuspendedList;

	if (o) {
		do {
			if (arg3) {
				if (INSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			} else {
				if (OUTSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			}

			o = o->mNextItem;
		} while (o);
	}

	o = EnvironmentalObjectList;

	if (o) {
		do {
			if (arg3) {
				if (INSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			} else {
				if (OUTSIDE(o->mPos.vx, o->mPos.vz, o->mPos.vy))
					SET(o);
			}

			o = o->mNextItem;
		} while (o);
	}

	q = NonRenderedBitList;

	if (q) {
		do {
			if (q->mType == 2) {
				if (arg3) {
					if (INSIDE(q->mPos.vx, q->mPos.vz, q->mPos.vy)) {
						if (arg2)
							((CLight *)q)->mOn = 1;
						else if (GNumberOfPlayers == 1)
							((CLight *)q)->mOn = 0;
					}
				} else {
					if (OUTSIDE(q->mPos.vx, q->mPos.vz, q->mPos.vy)) {
						if (arg2)
							((CLight *)q)->mOn = 1;
						else if (GNumberOfPlayers == 1)
							((CLight *)q)->mOn = 0;
					}
				}
			}

			q = q->mNext;
		} while (q);
	}
}

/* Utils_CalculateSpatialAttenuation__FRC7CVectorii (0x800489e4) */
/* Demo twin; locals from its MAIN.SYM. Volume for a sound at pos heard from the
   current camera: full inside near, silent past far, linear between; then panned left/right by
   the source's heading in camera space (quieter and flagged when it is behind). The sine table is
   a sized extern so its address is one `la` macro: reorg then fills the branch delay slot with the
   index shift, as retail does. */
int Utils_CalculateSpatialAttenuation(const CVector &pos, int near, int far) {
	int vol;
	CVector cam_pos(MikeCamera[Camera_CurrentMikeCamera].Position.vx << 12,
	                MikeCamera[Camera_CurrentMikeCamera].Position.vy << 12,
	                MikeCamera[Camera_CurrentMikeCamera].Position.vz << 12);
	int d = Utils_CrapDist(pos, cam_pos);
	CVector centre;
	short heading;
	uint behind;
	uint left;
	uint right;

	if (near >= d)
		return 0x0fff0fff;

	if (d >= far)
		return 0;

	vol = (((far - d) << 12) - (far - d)) / far;
	geom_ld_mat(pc_geom.rot, &MikeCamera[Camera_CurrentMikeCamera].View);

	centre.vx = (pos.vx >> 12) - MikeCamera[Camera_CurrentMikeCamera].Position.vx;
	centre.vy = (pos.vy >> 12) - MikeCamera[Camera_CurrentMikeCamera].Position.vy;
	centre.vz = (pos.vz >> 12) - MikeCamera[Camera_CurrentMikeCamera].Position.vz;
	{
		/* into camera space: the saturated 16-bit results, as words */
		int16_t v[3];
		int32_t w[3];
		GeomAcc r;

		geom_ld_lv(v, &centre);
		r = geom_mvmva(pc_geom.rot, 0, v[0], v[1], v[2], 12, 0, 0);
		geom_load_vector(w, r.ir[0], r.ir[1], r.ir[2]);
		geom_st_lv(&centre, w);
	}

	if (!(centre.vx | centre.vz))
		return vol | (vol << 16);

	heading = (1024 - ratan2(centre.vz, centre.vx)) & 4095;
	behind = 0;

	if (heading > 1024 && heading < 3072) {
		behind = 0x80000000;
		vol -= vol >> 4;
	}

	if (heading < 2048) {
		left = vol - ((vol * rcossin_tbl[heading].s) >> 12);
		right = vol;
	} else {
		left = vol;
		right = left + ((vol * rcossin_tbl[heading].s) >> 12);
	}

	return left | (right << 16) | behind;
}

/* FUN_80048c58 (0x80048c58) */
void FUN_80048c58(void) {
	int i;

	for (i = 2; i != -1; i--)
		;
}

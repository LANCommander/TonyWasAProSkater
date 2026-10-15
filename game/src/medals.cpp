/* MEDALS.CPP: CMedal, a spinning, bobbing medal model (a CBody), and Medal_Create, which
   places the competition medals (as CPowerUps). Declarations in game/medals.h. */
#include "game/medals.h"
#include "game/powerup.h"
#include "game/career.h"
#include "game/camera.h"
#include "game/bruce.h"
#include "game/globals.h"
#include "game/m3d.h"
#include "game/psyq.h"

#define SIN(a) (rcossin_tbl[(a) & 4095].s)

/* _._6CMedal (0x80073c94) */
/* Frees the medal's extras; CBody's destructor does the rest. */
CMedal::~CMedal() {
	CMedal::DeleteStuff();
}

/* DeleteStuff__6CMedal (0x80073cf0) */
/* A medal has nothing extra to free. */
void CMedal::DeleteStuff() {}

/* DoPhysics__6CMedal (0x80073cf8) */
void CMedal::DoPhysics() {
	mAngles += mAngVel;
	mAngles.Mask();

	mPos = mOrgPos;
	mPos.vy += ((mWobbleAmplitude * SIN(mT)) / 4096) << 12;
	mT += mWobbleSpeed;
}

/* FUN_80073da8 (0x80073da8) */
/* The medal's physics, called (not dispatched) on an untyped object. */
void FUN_80073da8(void *p) {
	((CMedal *)p)->CMedal::DoPhysics();
}

/* Medal_Create__F10EMedalTypeiiiii (0x80073dc8) */
/* C++. A medal (types 5..7: gold, silver, bronze powerups 1636..1638) placed at (x, y, z) in the
   current camera's frame, scaled a, wobbling with amplitude b. */
CPowerUp *Medal_Create(EMedalType type, int x, int y, int z, int a, int b) {
	SVECTOR v;
	VECTOR o;
	CVector pos;
	CPowerUp *p;
	short sa;
	short *q;

	v.vx = x;
	v.vy = y;
	v.vz = z;
	{
		/* v through the camera's transform, unshifted */
		GeomAcc r;

		geom_ld_mat(pc_geom.rot, &pCurrentCamera->Transform);
		r = geom_mvmva(pc_geom.rot, 0, v.vx, v.vy, v.vz, 0, 0, 0);
		geom_st_lv(&o, r.mac);
	}

	pos.vx = o.vx + (pCurrentCamera->Position.vx << 12);
	pos.vy = o.vy + (pCurrentCamera->Position.vy << 12);
	pos.vz = o.vz + (pCurrentCamera->Position.vz << 12);

	switch (type) {
		case 5:
			p = new CPowerUp(1636, pos, ZeroVector, 0, 0xffff);
			break;
		case 6:
			p = new CPowerUp(1637, pos, ZeroVector, 0, 0xffff);
			break;
		case 7:
			p = new CPowerUp(1638, pos, ZeroVector, 0, 0xffff);
			break;
	}

	q = &p->mScale.vx;
	p->mWobbleSpeed = 80;
	sa = a;
	p->mFlags |= 0x200;
	p->mScale.vx = sa;
	q[1] = sa;
	q[2] = sa;
	p->mWobbleAmplitude = b;
	p->mMaxWobbleAmplitude = b;
	return p;
}

/* FUN_80073f9c (0x80073f9c) */
/* C++: whether the current skater has a medal on all three competition levels. */
extern "C" int FUN_80073f9c(void) {
	int c = 0;
	int i = 0;
	CBruce *g = GSkater[0];

	for (; i < 10; i++) {
		if (Levels[i].isCompetition) {
			if (GCareer.CharacterProgress[g->mSkaterIndex].Tapes[i] & 224)
				c++;
		}
	}

	return c == 3;
}

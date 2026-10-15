/* Medal_Create__F10EMedalTypeiiiii (0x80073dc8) */
/* C++. A medal (types 5..7: gold, silver, bronze powerups 1636..1638) placed at (x, y, z) in the
   current camera's frame, scaled a, wobbling with amplitude b. */
#include "game/powerup.h"
#include "game/career.h"
#include "game/camera.h"

extern SCamera *pCurrentCamera;
extern CVector ZeroVector;

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

/* Tape_Create__Fiiiiiiii (0x80073a64) */
/* C++. A video tape pickup (powerup 18) flying from (x, y, z) to (e, f, g), both in the current
   camera's frame. */
#include "game/tapes.h"
#include "game/camera.h"

extern SCamera *pCurrentCamera;
extern CVector ZeroVector;

CTape *Tape_Create(int x, int y, int z, int p, int e, int f, int g, int h) {
	SVECTOR v;
	VECTOR o;
	CVector pos1, pos2;
	CTape *t;

	{
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

		pos1.vx = o.vx + (pCurrentCamera->Position.vx << 12);
		pos1.vy = o.vy + (pCurrentCamera->Position.vy << 12);
		pos1.vz = o.vz + (pCurrentCamera->Position.vz << 12);
	}

	{
		v.vx = e;
		v.vy = f;
		v.vz = g;
		o.vx = 0;
		o.vy = 0;
		o.vz = 0;
		{
			/* v through the camera's transform, unshifted */
			GeomAcc r;

			geom_ld_mat(pc_geom.rot, &pCurrentCamera->Transform);
			r = geom_mvmva(pc_geom.rot, 0, v.vx, v.vy, v.vz, 0, 0, 0);
			geom_st_lv(&o, r.mac);
		}

		pos2.vx = o.vx + (pCurrentCamera->Position.vx << 12);
		pos2.vy = o.vy + (pCurrentCamera->Position.vy << 12);
		pos2.vz = o.vz + (pCurrentCamera->Position.vz << 12);
	}

	t = new CTape(18, pos1, pos2, p, h, ZeroVector, 0, 0xffff);

	t->mWobbleAmplitude = 10;
	t->mMaxWobbleAmplitude = 10;
	t->mWobbleSpeed = 80;
	t->mFlags |= 0x200;
	return t;
}

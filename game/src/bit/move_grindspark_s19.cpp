/* Move__11CGrindSpark (0x8002780c) */
#include "game/bit.h"
#include "game/ob.h"

void CGrindSpark::Move() {
	int px, py, pz, nx, ny, nz, sx, sy, sz;

	px = mPos.vx;
	py = mPos.vy;
	pz = mPos.vz;
	nx = px + mVel.vx;
	ny = py + mVel.vy;
	nz = pz + mVel.vz;
	sx = mpParent->mPos.vx;
	sy = mpParent->mPos.vy;
	sz = mpParent->mPos.vz;
	mOldPos.vx = px;
	mOldPos.vy = py;
	mOldPos.vz = pz;
	mEnd.vx = px + sx;
	mEnd.vy = py + sy;
	mEnd.vz = pz + sz;
	mPos.vx = nx;
	mPos.vy = ny;
	mPos.vz = nz;
	mStart.vx = nx + sx;
	mStart.vy = ny + sy;
	mStart.vz = nz + sz;
	nx = mVel.vx + mAcc.vx;
	ny = mVel.vy + mAcc.vy;
	nz = mVel.vz + mAcc.vz;
	nx -= nx >> 3;
	ny -= ny >> 3;
	nz -= nz >> 3;
	mVel.vx = nx;
	mVel.vy = ny;
	mVel.vz = nz;
	mAge++;

	if (mAge >= mLifetime)
		Die();
}

/* SquirtPos__5CBodyPs (0x800439ac) */
/* C++ (OB.CPP). Reads a whole-unit position (three ints) from object data into the position and
   tween position (as 20.12 fixed point); returns the data pointer past it. */
#include "game/ob.h"

short *CBody::SquirtPos(short *p_info) {
	int *p = (int *)p_info;

	mPos.vx = mTweenPos.vx = p[0] << 12;
	mPos.vy = mTweenPos.vy = p[1] << 12;
	mPos.vz = mTweenPos.vz = p[2] << 12;
	return (short *)(p + 3);
}

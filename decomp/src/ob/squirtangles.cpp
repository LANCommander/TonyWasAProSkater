/* SquirtAngles__5CBodyPs (0x800439ec) */
/* C++ (OB.CPP). Reads the three angles from object data; returns the data pointer past them. */
#include "game/ob.h"

short *CBody::SquirtAngles(short *p_info) {
	mAngles.vx = *p_info++;
	mAngles.vy = *p_info++;
	mAngles.vz = *p_info++;
	return p_info;
}

/* __5CItem (0x80043550) */
/* C++ (OB.CPP). A new item starts at unit scale, lit by the default light. */
#include "game/ob.h"

extern SLight M3d_DefaultLight;

CItem::CItem() {
	mScale.vx = 4096;
	mScale.vy = 4096;
	mScale.vz = 4096;
	mpLight = &M3d_DefaultLight;
}

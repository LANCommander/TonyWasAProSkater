/* ShadowOn__5CBody (0x80043ae0) */
/* C++ (OB.CPP). Turns the body's shadow on. */
#include "game/ob.h"

void CBody::ShadowOn() {
	mCBodyFlags |= 8;
}

/* _._5CBody (0x80043860) */
/* C++ (OB.CPP). Deletes the body's shadow bit; CItem's destructor does the rest. */
#include "game/ob.h"
#include "game/bit.h"

CBody::~CBody() {
	if (mpShadow)
		delete mpShadow;
}

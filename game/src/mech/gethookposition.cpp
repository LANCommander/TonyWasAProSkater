/* GetHookPosition__6CBruceR7CVectorUc (0x8004274c) */
/* C++ (MECH.CPP). The world position of hook point Index on the skater's model. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void M3dUtils_GetHookPosition(VECTOR *, CSuper *, int);

void CBruce::GetHookPosition(CVector &Pos, uchar Index) {
	M3dUtils_GetHookPosition((VECTOR *)&Pos, this, Index);
}

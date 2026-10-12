/* AI__11CBackground (0x8007828c) */
/* C++ (BACKGRND.CPP). A background object just spins at its angular velocity. */
#include "game/backgrnd.h"

void CBackground::AI() {
	mAngles += mAngVel;
}

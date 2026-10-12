/* QFromYRot__Fs (0x80077568) */
/* C++ (QUAT.CPP). Exact demo twin: quaternion for a rotation about one axis. Locals from MAIN.SYM. */
#include "game/vector.h"

struct SSinCos {
	short s, c;
};

extern SSinCos rcossin_tbl[1]; /* declared <= 8 bytes: one la, as retail */

CQuat QFromYRot(short angle) {
	int cos2 = (rcossin_tbl + ((angle >> 1) & 4095))->c;
	int sin2 = (rcossin_tbl + ((angle >> 1) & 4095))->s;

	return CQuat(0, sin2, 0, cos2);
}

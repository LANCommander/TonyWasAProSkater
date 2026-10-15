/* FUN_80065fd4 (0x80065fd4) */
/* C++ (FileLoader fields, not int offsets past the vptr: 64-bit). */
#include "game/shellutils.h"

extern "C" int FUN_80065fd4(FileLoader *p) {
	return p->mCur == p->mNum;
}

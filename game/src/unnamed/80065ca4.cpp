/* FUN_80065ca4 (0x80065ca4) */
/* C++ (FileLoader fields, not int offsets past the vptr: 64-bit). */
#include "game/shellutils.h"
#include "game/fileio.h"
#include "game/spool.h"

extern int Loading;

extern "C" void FUN_8006582c(FileLoader *p);

extern "C" void FUN_80065ca4(FileLoader *p) {
	FileIO_Continue();

	if (p->mMode == 1)
		Spool_Continue();

	if (p->mCur < p->mNum && Loading == 0)
		FUN_8006582c(p);
}

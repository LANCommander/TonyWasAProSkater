/* FUN_80065d14 (0x80065d14) */
/* C++ (FileLoader fields, not int offsets past the vptr: 64-bit). */
#include "game/shellutils.h"
#include "game/fileio.h"
#include "game/spool.h"

extern "C" void FUN_8006582c(FileLoader *p);

extern "C" void FUN_80065d14(FileLoader *p) {
	while (p->mCur < p->mNum)
		FUN_8006582c(p);

	if (p->mMode == 1)
		Spool_Sync();
	else
		FileIO_Sync();
}

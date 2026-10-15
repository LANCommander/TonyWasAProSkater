/* FUN_80065d80 (0x80065d80) */
/* C++ (FileLoader fields, not int offsets past the vptr: 64-bit). */
#include "game/shellutils.h"
#include "game/fileio.h"
#include "game/spool.h"

extern "C" void FUN_80065d80(FileLoader *p) {
	int i;

	if (p->mMode == 1)
		Spool_Sync();
	else
		FileIO_Sync();

	for (i = 0; i < p->mNum; i++)
		p->unload(i);

	p->mCur = 0;
}

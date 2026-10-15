/* unload__10FileLoaderi (0x80065e04) */
/* C++. Frees file i of the set: a heap file's buffer, a PRE archive, or a spooled PSX region. */
#include "game/shellutils.h"
#include "game/pre.h"
#include "game/mem.h"
#include "game/spool.h"

extern PREManager *SpoolerBlocksFileIO;

void Spool_ClearPSX(char *pName);

void FileLoader::unload(int i) {
	SLoadEntry *e = &mEntry[i];

	if (e->Loaded) {
		if (e->Type == 0) {
			Mem_Delete(e->pData);

			e->pData = 0;
		}

		if (e->Type == 2) {
			/* (with no PRE archive loaded retail looked through a null manager in low RAM) */
			if (SpoolerBlocksFileIO)
				SpoolerBlocksFileIO->unloadPRE(e->Name);
		} else if (e->Type == 1) {
			ProcessNewPSX(e->Handle);
			Spool_ClearPSX(e->Name);
			Spool_Sync();

			e->Handle = -1;
		}

		e->Loaded = 0;
	}
}

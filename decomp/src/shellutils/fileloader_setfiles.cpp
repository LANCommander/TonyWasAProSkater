/* FUN_80065594 (0x80065594) */
/* C++ (SHELLUTILS.CPP, THPS1-only; no demo twin). A front-end file loader object (vtable
   0x800a2130, 32 entries of 36 bytes). This is its "load this set of files" call: it syncs
   any pending read, keeps entries that are still wanted and loaded (compacting them to the
   front), unloads the rest (FileLoader::unload), appends the new names as unloaded entries, and
   blanks the unused tail. FrontLoader_SetFiles calls it with a per-screen file list. */

#include "game/fileio.h"
#include "game/psyq.h"
#include "game/shellutils.h"

void Spool_Sync();

void FileLoader::setFiles(char **names, int num) {
	int keep[32];
	int i;
	int j;
	int n;

	if (mMode == 1)
		Spool_Sync();
	else
		FileIO_Sync();

	for (int k = 0; k < 32; k++)
		keep[k] = 0;

	n = 0;

	for (i = 0; i < mNum; i++) {
		for (j = 0; j < num; j++)
			if (!strcmp(mEntry[i].Name, names[j]))
				break;

		if (j < num && mEntry[i].Loaded) {
			keep[j] = 1;
			mEntry[n++] = mEntry[i];
		} else
			unload(i);
	}

	for (int f = 0; f < num; f++) {
		if (!keep[f]) {
			strcpy(mEntry[n].Name, names[f]);

			mEntry[n].Loaded = 0;
			n++;
		}
	}

	mNum = n;

	for (int e = n; e < 32; e++) {
		strcpy(mEntry[e].Name, "URB");

		mEntry[e].Loaded = 0;
	}

	mCur = 0;
	mMode = 0;
	FileIO_SetSectorsPerFrame(4);
}

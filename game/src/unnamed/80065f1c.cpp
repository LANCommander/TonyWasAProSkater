/* FUN_80065f1c (0x80065f1c) */
/* C++. Takes file `name` out of a FileLoader's set (FrontLoader_TakeFile): its buffer, which the
   caller now owns (the entry forgets it and counts as not loaded), or 0 if the set has no such
   file. */
#include "game/shellutils.h"

#include "game/psyq.h"

extern "C" void *FUN_80065f1c(FileLoader *p, char *name) {
	int i;
	void *r = 0;

	for (i = 0; i < p->mNum; i++) {
		if (strcmp(p->mEntry[i].Name, name) == 0) {
			r = p->mEntry[i].pData;
			p->mEntry[i].pData = 0;
			p->mEntry[i].Loaded = 0;
			break;
		}
	}

	return r;
}

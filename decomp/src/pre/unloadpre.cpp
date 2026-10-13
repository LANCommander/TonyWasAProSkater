/* unloadPRE__10PREManagerPc (0x80066338) */
/* C++ (PRE.CPP). Frees the archive loaded under pre_name and clears its slot. */
#include "game/pre.h"
#include "game/psyq.h"

void Mem_Delete(void *);

void PREManager::unloadPRE(char *pre_name) {
	for (int i = 0; i < 16; i++) {
		if (!strcmp(mPreName[i], pre_name)) {
			strcpy(mPreName[i], "");

			if (mPre[i])
				Mem_Delete(mPre[i]);

			mPre[i] = 0;
			return;
		}
	}
}

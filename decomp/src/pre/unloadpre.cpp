/* unloadPRE__10PREManagerPc (0x80066338) */
/* C++ (PRE.CPP). Frees the archive loaded under pre_name and clears its slot. */
#include "game/pre.h"

extern char DAT_800d1dbc[]; /* "" */

extern "C" int FUN_8008f6fc(char *, char *); /* strcmp */
void Mem_Delete(void *);

void PREManager::unloadPRE(char *pre_name) {
	for (int i = 0; i < 16; i++) {
		if (!FUN_8008f6fc(mPreName[i], pre_name)) {
			mPreName[i][0] = DAT_800d1dbc[0];

			if (mPre[i])
				Mem_Delete(mPre[i]);

			mPre[i] = 0;
			return;
		}
	}
}

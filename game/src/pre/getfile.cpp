/* getFile__10PREManagerPcPi (0x800663d8) */
/* C++ (PRE.CPP). Looks pName up in every loaded .PRE archive; the last loaded slot's answer wins
   (null if that archive doesn't hold it). */
#include "game/pre.h"

char *PREManager::getFile(char *pName, int *pSize) {
	char *pFile = 0;

	for (int i = 0; i < 16; i++)
		if (mPre[i])
			pFile = parsePRE(mPre[i], pName, pSize);

	return pFile;
}

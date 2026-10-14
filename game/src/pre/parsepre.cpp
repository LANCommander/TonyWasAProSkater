/* parsePRE__10PREManagerPcT1Pi (0x80066460) */
/* C++ (PRE.CPP). Walks a .PRE archive (count, then per file: word-aligned name, size, word-aligned
   data) and returns the data of the file called pName, storing its size, or null. */
#include "game/pre.h"
#include "game/psyq.h"

char *PREManager::parsePRE(char *pPre, char *pName, int *pSize) {
	int numFiles = *(int *)pPre;

	pPre += 4;

	for (int i = 0; i < numFiles; i++) {
		char *pFileName = pPre;
		int size;

		pPre += (strlen(pPre) + 4) & ~3;
		size = *(int *)pPre;
		pPre += 4;

		if (!strcmp(pFileName, pName)) {
			*pSize = size;
			return pPre;
		}

		pPre += (size + 3) & ~3;
	}

	return 0;
}

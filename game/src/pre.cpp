/* PRE.CPP: PREManager, the loaded .PRE archives: spooling, unloading, parsing and looking up
   their files. Declarations in game/pre.h. */
#include "game/pre.h"
#include "game/fileio.h"
#include "game/mem.h"
#include "game/psyq.h"
#include "game/globals.h"

void __dl__6CClassPv(void *p);

/* spoolPRE__10PREManagerPc (0x8006623c) */
/* Demo twin; locals from its MAIN.SYM. Loads a .PRE archive into the first free slot
   and records its name (THPS1 copies the name with an inline loop instead of strcpy). */
void PREManager::spoolPRE(char *pre_name) {
	int slot;

	for (slot = 0; slot < 16; slot++)
		if (!mPre[slot])
			break;

	if (slot == 16) {
		printf("\nOut of PREManager slots");
		return;
	}

	if ((mPre[slot] = Mem_NewTop(FileIO_Open(pre_name))))
		FileIO_Load(mPre[slot]);
	else
		printf("\nMemory error loading %s", pre_name);

	{
		char *d = (char *)(slot * 16 + (intptr_t)this);
		char *s = pre_name;

		d += (char *)mPreName - (char *)this;

		while (*s)
			*d++ = *s++;

		*d = 0;
	}
}

/* unloadPRE__10PREManagerPc (0x80066338) */
/* Frees the archive loaded under pre_name and clears its slot. */
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

/* getFile__10PREManagerPcPi (0x800663d8) */
/* Looks pName up in every loaded .PRE archive; the last loaded slot's answer wins
   (null if that archive doesn't hold it). */
char *PREManager::getFile(char *pName, int *pSize) {
	char *pFile = 0;

	for (int i = 0; i < 16; i++)
		if (mPre[i])
			pFile = parsePRE(mPre[i], pName, pSize);

	return pFile;
}

/* parsePRE__10PREManagerPcT1Pi (0x80066460) */
/* Walks a .PRE archive (count, then per file: word-aligned name, size, word-aligned
   data) and returns the data of the file called pName, storing its size, or null. */
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

/* FUN_80066524 (0x80066524) */
void FUN_80066524(char *p, int f) {
	*(char **)(p + 0) = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_80066558 (0x80066558) */
void FUN_80066558(char *p, int f) {
	*(char **)(p + 0) = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

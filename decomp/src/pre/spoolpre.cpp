/* spoolPRE__10PREManagerPc (0x8006623c) */
/* C++ (PRE.CPP). Demo twin; locals from its MAIN.SYM. Loads a .PRE archive into the first free slot
   and records its name (THPS1 copies the name with an inline loop instead of strcpy). */
#include "game/pre.h"

int FileIO_Open(const char *);

extern "C" {
	char *Mem_NewTop__FUi(int);
	void FileIO_Load__FPv(char *);
	void FUN_8008c190(char *, ...);
}

void PREManager::spoolPRE(char *pre_name) {
	int slot;

	for (slot = 0; slot < 16; slot++)
		if (!mPre[slot])
			break;

	if (slot == 16) {
		FUN_8008c190("\nOut of PREManager slots");
		return;
	}

	if ((mPre[slot] = Mem_NewTop__FUi(FileIO_Open(pre_name))))
		FileIO_Load__FPv(mPre[slot]);
	else
		FUN_8008c190("\nMemory error loading %s", pre_name);

	{
		char *d = (char *)(slot * 16 + (int)this);
		char *s = pre_name;

		d += 68;

		while (*s)
			*d++ = *s++;

		*d = 0;
	}
}

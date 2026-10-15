/* spoolPRE__10PREManagerPc (0x8006623c) */
/* C++ (PRE.CPP). Demo twin; locals from its MAIN.SYM. Loads a .PRE archive into the first free slot
   and records its name (THPS1 copies the name with an inline loop instead of strcpy). */
#include "game/fileio.h"
#include "game/mem.h"
#include "game/pre.h"
#include "game/psyq.h"

int FileIO_Open(const char *);

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

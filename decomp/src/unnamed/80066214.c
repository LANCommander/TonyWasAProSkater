/* FUN_80066214 (0x80066214) */
#include "names.h"

void PREManager_spoolPRE(void *, char *);
void FileIO_Sync(void);

void FUN_80066214(void *pm, char *name) {
	PREManager_spoolPRE(pm, name);
	FileIO_Sync();
}

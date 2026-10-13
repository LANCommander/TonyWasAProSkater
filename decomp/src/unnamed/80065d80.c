/* FUN_80065d80 (0x80065d80) */
#include "names.h"

void Spool_Sync(void);
void FileIO_Sync(void);
void FUN_80065e04(int *p, int i);

void FUN_80065d80(int *p) {
	int i;

	if (p[1] == 1)
		Spool_Sync();
	else
		FileIO_Sync();

	for (i = 0; i < p[2]; i++)
		FUN_80065e04(p, i);

	p[3] = 0;
}

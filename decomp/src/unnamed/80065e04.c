/* FUN_80065e04 (0x80065e04) */
#include "names.h"
extern int SpoolerBlocksFileIO;

void Mem_Delete(int);
void ProcessNewPSX(int);
void Spool_ClearPSX(char *);
void Spool_Sync(void);
void PREManager_unloadPRE(int, char *);

void FUN_80065e04(char *a, int b) {
	int o = b * 36;
	int *p30 = (int *)(a + 0x30);
	int *s, *q;
	char *t, *t1, *t2, *t3;

	if (*(int *)((char *)p30 + o)) {
		t1 = a + 0x28;
		s = (int *)(t1 + o);

		if (*s == 0) {
			q = (int *)(a + 0x10);
			q = (int *)((char *)q + o);
			Mem_Delete(*q);

			*q = 0;
		}

		if (*s == 2) {
			PREManager_unloadPRE(SpoolerBlocksFileIO, (char *)(o + (int)a + 20));
		} else if (*s == 1) {
			q = (int *)(a + 0x24);
			q = (int *)((char *)q + o);
			ProcessNewPSX(*q);
			Spool_ClearPSX((char *)(o + (int)a) + 20);
			Spool_Sync();

			*q = -1;
		}

		t = (char *)(b * 36);
		*(int *)((int)p30 + (int)t) = 0;
	}
}

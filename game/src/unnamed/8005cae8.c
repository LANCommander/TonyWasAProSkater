/* Front_FreeSpritesPRE__Fv (0x8005cae8) */
#include "names.h"

struct VT {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	struct VT *vt;
};

extern struct O *SpoolerBlocksFileIO;

void printf(char *a);

void Front_FreeSpritesPRE(void) {
	if (SpoolerBlocksFileIO)
		printf("\nNo PREMgr to delete");

	if (SpoolerBlocksFileIO)
		SpoolerBlocksFileIO->vt->fn((char *)SpoolerBlocksFileIO + SpoolerBlocksFileIO->vt->off, 3);

	SpoolerBlocksFileIO = 0;
}

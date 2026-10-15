/* Front_LoadSpritesPRE__Fv (0x8005ca88) */
#include "game/classsizes.h"
#include "names.h"

extern char *SpoolerBlocksFileIO;

void printf(char *a);
char *__nw__6CClassUi(unsigned n);
char *FUN_8006613c(char *p);
void FUN_80066214(char *p, char *a);

void Front_LoadSpritesPRE(void) {
	char *p;

	if (SpoolerBlocksFileIO)
		printf("\nPREMgr already loaded");

	p = __nw__6CClassUi(SIZEOF_PREMANAGER);

	if (p)
		p = FUN_8006613c(p);

	SpoolerBlocksFileIO = p;
	FUN_80066214(p, "Sprites.pre");
}

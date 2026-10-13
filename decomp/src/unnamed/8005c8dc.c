/* FrontLoader_SetFiles__Fi (0x8005c8dc) */
#include "names.h"

struct T {
	char pad[128];
	int f;
};

extern struct T DAT_800be478[];
extern char DAT_800bec34[];

int DAT_800d1cac;

void FrontLoader_CheckSet(void);
void FUN_80065594(char *a, struct T *b, int c);

void FrontLoader_SetFiles(int a) {
	DAT_800d1cac = a;
	FrontLoader_CheckSet();
	FUN_80065594(DAT_800bec34, &DAT_800be478[a], DAT_800be478[a].f);
}

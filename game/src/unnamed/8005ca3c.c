/* FrontLoader_TakeFile__FPc (0x8005ca3c) */
#include "names.h"

extern char DAT_800bec34[];

char *FUN_80065f1c(char *p, char *name);

char *FrontLoader_TakeFile(char *name) {
	return FUN_80065f1c(DAT_800bec34, name);
}

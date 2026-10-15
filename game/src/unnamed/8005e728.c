/* FUN_8005e728 (0x8005e728) */
#include "game/padbuttons.h"

extern char Pad[];

struct VerticalMenu;

void FUN_800648c0(struct VerticalMenu *a, int b, int c);

void FUN_8005e728(struct VerticalMenu *a) {
	FUN_800648c0(a, 1, *(int *)(Pad + PAD_SIZE + 0x14c) == 0);
}

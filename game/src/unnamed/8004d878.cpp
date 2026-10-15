/* Mem_GetFree__Fc (0x8004d878) */
/* MEM: the bytes heap i has left. */
#include "game/mem.h"

int Mem_GetFree(char i) {
	return DAT_800be09c[(uchar)i].End - DAT_800be09c[(uchar)i].Start - Used[(uchar)i];
}

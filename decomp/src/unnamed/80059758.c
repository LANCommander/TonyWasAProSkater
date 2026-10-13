/* Front_FreeSkaterPack__Fv (0x80059758) */
#include "names.h"

int DAT_800d1c80;
short DAT_800d1c94;
short DAT_800d1c98;

void Front_SelectSkaterPack(int a);
void Mem_Delete(int a);
void Pack_Unpack(short a, short b);

void Front_FreeSkaterPack(void) {
	Front_SelectSkaterPack(-1);
	Mem_Delete(DAT_800d1c80);
	Pack_Unpack(DAT_800d1c94, DAT_800d1c98);
}

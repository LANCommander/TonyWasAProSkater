/* Front_SelectSkaterPack__Fi (0x80059618) */
#include "names.h"

int DAT_800d1c5c;

struct TSPack;

extern struct TSPack *Pack_pFree;
extern struct TSPack *Pack_pFull;

struct TSPack *DAT_800d1c8c;
struct TSPack *DAT_800d1c90;
struct TSPack *DAT_800d1c84;
struct TSPack *DAT_800d1c88;

void Front_SelectSkaterPack(int a) {
	if (DAT_800d1c5c == -1) {
		DAT_800d1c8c = Pack_pFree;
		DAT_800d1c90 = Pack_pFull;
	} else {
		DAT_800d1c84 = Pack_pFree;
		DAT_800d1c88 = Pack_pFull;
	}

	DAT_800d1c5c = a;

	if (a == -1) {
		Pack_pFree = DAT_800d1c8c;
		Pack_pFull = DAT_800d1c90;
	} else {
		Pack_pFree = DAT_800d1c84;
		Pack_pFull = DAT_800d1c88;
	}
}

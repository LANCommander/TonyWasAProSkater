/* FUN_8003e934 (0x8003e934) */
#include "names.h"

struct VT {
	char pad[16];
	short off;
	short pad2;
	void (*fn)(char *);
};

void Shatter_Item(char *p, int a, int b);
void CItem_Flash(char *p, int a, int b, int c, int d);

int FUN_8003e934(char *p, int a) {
	p[415] = p[415] + 1;

	if (*(unsigned short *)(p + 470)) {
		if (*(short *)(p + 230) > 0) {
			*(short *)(p + 230) = *(unsigned short *)(p + 230) - a;

			if (*(short *)(p + 230) <= 0) {
				struct VT *vt = *(struct VT **)(p + 72);

				vt->fn(p + vt->off);
				Shatter_Item(p, 0, 1);
			} else
				CItem_Flash(p, 3, 255, 255, 255);
		}
	}

	return 1;
}

/* Pack_DefragmentFully__Fv (0x8004a8cc) */
#include "names.h"

typedef struct TSPack {
	unsigned short x, y, w, h;
	struct TSPack *next;
} TSPack;

extern TSPack *Pack_pFree;
extern int Pack_Width, ZapCalls;

void Pack_KillSlot(TSPack *);
void Pack_Zap(int, int, int, int);
int Pack_Merge(TSPack *);

void Pack_DefragmentFully(void) {
	TSPack *p;
	TSPack *n;
	int m;

	if (Pack_pFree) {
		ZapCalls = 0;
		p = Pack_pFree;

		do {
			n = p->next;
			Pack_KillSlot(p);

			p = n;
		} while (p);

		Pack_pFree = 0;

		switch (Pack_Width) {
			case 320:
				Pack_Zap(320, 256, 64, 256);
				Pack_Zap(Pack_Width + 64, 256, 128, 256);
				Pack_Zap(Pack_Width + 192, 256, 512, 256);
				Pack_Zap(Pack_Width, 0, 64, 256);
				Pack_Zap(Pack_Width + 64, 0, 128, 256);
				Pack_Zap(Pack_Width + 192, 0, 512, 256);
				break;
			case 640:
				Pack_Zap(640, 256, 128, 256);
				Pack_Zap(Pack_Width + 128, 256, 256, 256);
				Pack_Zap(Pack_Width, 0, 128, 256);
				Pack_Zap(Pack_Width + 128, 0, 256, 256);
				break;
			default:
				Pack_Zap(Pack_Width, 256, 1024 - Pack_Width, 256);
				Pack_Zap(Pack_Width, 0, 1024 - Pack_Width, 256);
				break;
		}

		do {
			p = Pack_pFree;
			m = 0;

			while (p) {
				m |= Pack_Merge(p);
				p = p->next;
			}
		} while (m);
	}
}

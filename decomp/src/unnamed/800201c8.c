/* FUN_800201c8 (0x800201c8) */
#include "names.h"

struct VT {
	char pad[0x10];
	short off;
	short pad2;
	void (*fn)(char *);
};

typedef struct {
	unsigned short f0;
	char p0[0x48 - 2];
	struct VT *vt;
	char p1[0x156 - 0x4c];
	unsigned short f156;
	short f158;
	char p2[2];
	unsigned short f15c;
	char p3[2];
	int f160;
} B;

extern int TTime;

void CPowerUp_DontDisplay(B *);

void FUN_800201c8(B *p) {
	p->f160++;

	if (p->f15c != 0xffff) {
		if (p->f15c != 0)
			p->f15c--;

		if (p->f15c < 60) {
			p->f158 = (p->f156 * p->f15c) / 60;
		}

		if ((unsigned short)(p->f15c - 31) < 29) {
			if (TTime & 2)
				p->f0 &= 0xfffe;
			else
				CPowerUp_DontDisplay(p);
		}

		if (p->f15c < 31) {
			if (TTime & 1)
				p->f0 &= 0xfffe;
			else
				CPowerUp_DontDisplay(p);
		}

		if (p->f15c == 0)
			p->vt->fn((char *)p + p->vt->off);
	}
}

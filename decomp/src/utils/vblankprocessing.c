/* Utils_VblankProcessing__Fv (0x8004777c) */
#include "names.h"

typedef struct {
	unsigned tag;
	unsigned char r0, g0, b0, code;
	short x0, y0, x1, y1, x2, y2, x3, y3;
} POLY_F4;

typedef struct {
	unsigned tag;
	unsigned code[1];
} DR_TPAGE;

volatile int GameFade;

extern int SFX_ReverbDepthSetTimer;

int FUN_8008c468(int);
void FUN_8008c880(void *);
void SFX_SetReverbDepth(int, int);

void Utils_VblankProcessing(void) {
	if (GameFade) {
		POLY_F4 p;
		DR_TPAGE m;

		m.tag = 0x01000000;
		m.code[0] = 0xe1000640;
		((unsigned char *)&p.tag)[3] = 5;
		GameFade--;
		p.code = 0x2a;

		if (GameFade & 1) {
			p.r0 = 0;
			p.g0 = 0;
			p.b0 = 8;
		} else {
			p.r0 = 8;
			p.g0 = 8;
			p.b0 = 0;
		}

		p.x0 = 0;
		p.x1 = 512;
		p.y0 = 0;
		p.y1 = 0;
		p.x2 = 0;
		p.x3 = 512;
		p.y2 = 240;
		p.y3 = 240;
		FUN_8008c468(0);
		FUN_8008c880(&m);
		FUN_8008c880(&p);
		FUN_8008c468(0);
	}

	if (SFX_ReverbDepthSetTimer) {
		if (--SFX_ReverbDepthSetTimer == 0)
			SFX_SetReverbDepth(32, 32);
	}
}

/* FUN_800502bc (0x800502bc) flags=-fstructure-noalias */
#include "names.h"

typedef struct P {
	unsigned tag;
	unsigned char r, g, b, code;
	short x0, y0;
	unsigned char u0, v0;
	short clut;
	short x1, y1;
	unsigned char u1, v1;
	short tpage;
	short x2, y2;
	unsigned char u2, v2;
	short pad2;
	short x3, y3;
	unsigned char u3, v3;
	short pad3;
} P;

int DAT_800d2274, DAT_800d2278, DAT_800d227c, DAT_800d226c, DAT_800d2270;
P *DAT_800d2280;

extern int GameFMV_RewindSwitch;
extern short GameFMV_StartX, GameFMV_StartY, GameFMV_EndX;
extern int pDoubleBuffer, OT_entry;
extern int DAT_800be3d0[];

void GameFMV_SetStartTrack(int);
void GameFMV_SyncFMV(int);
P *Mem_NewX(int, int, int);
void Mem_Delete(P *);
int VSync(int);
void GameFMV_StartFMV(int);
void GameFMV_StopFMV(void);

void FUN_800502bc(int a) {
	switch (a) {
		case 0:
			DAT_800d2274 = 0;
			DAT_800d2278 = 0;
			DAT_800d227c = 0;
			DAT_800d2280 = Mem_NewX(40, 1, 1);
			((unsigned char *)DAT_800d2280)[3] = 9;
			DAT_800d2280->code = 44;
			DAT_800d2280->r = 128;
			DAT_800d2280->g = 128;
			DAT_800d2280->b = 128;
			DAT_800d2280->u0 = 0;
			DAT_800d2280->v0 = 0;
			DAT_800d2280->u1 = 96;
			DAT_800d2280->v1 = 0;
			DAT_800d2280->u2 = 0;
			DAT_800d2280->v2 = 64;
			DAT_800d2280->u3 = 96;
			DAT_800d2280->v3 = 64;
			DAT_800d2280->code &= 253;
			DAT_800d2280->code &= 254;
			DAT_800d2280->tpage = 264;
			break;
		case 1:
			if (DAT_800d2274 == 0) {
				if (DAT_800d2278 == 0) {
					GameFMV_SetStartTrack(DAT_800be3d0[DAT_800d227c]);

					GameFMV_StartX = 512;
					GameFMV_EndX = 608;
					GameFMV_StartY = 0;
					GameFMV_StartFMV(1);

					DAT_800d226c = (VSync(1) + VSync(-1)) % 320;
					DAT_800d2270 = (VSync(1) + VSync(-1)) % 112;
				}

				DAT_800d2280->r = DAT_800d2278;
				DAT_800d2280->g = DAT_800d2278;
				DAT_800d2280->b = DAT_800d2278;
				DAT_800d2278 += 2;

				if (DAT_800d2278 >= 128) {
					DAT_800d2274 = a;
					DAT_800d2278 = 500;
				}
			} else if (DAT_800d2274 == a) {
				DAT_800d2278--;

				if (DAT_800d2278 == 0) {
					DAT_800d2274 = 2;
					DAT_800d2278 = 128;
				}
			} else if (DAT_800d2274 == 2) {
				DAT_800d2280->r = DAT_800d2278;
				DAT_800d2280->g = DAT_800d2278;
				DAT_800d2280->b = DAT_800d2278;
				DAT_800d2278 -= 2;

				if (DAT_800d2278 <= 0) {
					DAT_800d2274 = 0;
					DAT_800d2278 = 0;
					DAT_800d227c++;

					if (DAT_800d227c >= 10)
						DAT_800d227c = 0;

					GameFMV_StopFMV();
				}
			}

			GameFMV_SyncFMV(1);

			if (GameFMV_RewindSwitch) {
				DAT_800d2274 = 0;
				DAT_800d2278 = 0;
			}

			{
				P *q = DAT_800d2280;
				unsigned short x = DAT_800d226c, y = DAT_800d2270;
				unsigned *ot;

				ot = (unsigned *)(OT_entry * 4 + pDoubleBuffer);
				q->x0 = x;
				q->y0 = y;
				q->x1 = x + 192;
				q->y1 = y;
				q->x2 = x;
				q->y2 = y + 128;
				q->x3 = x + 192;
				q->y3 = y + 128;
				q->tag = (q->tag & 0xff000000) | (ot[35] & 0xffffff);
				ot[35] = (ot[35] & 0xff000000) | ((unsigned)q & 0xffffff);
			}

			break;
		case 2:
			GameFMV_StopFMV();
			Mem_Delete(DAT_800d2280);
			break;
	}
}

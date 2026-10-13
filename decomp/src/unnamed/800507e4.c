/* FUN_800507e4 (0x800507e4) flags=-fstructure-noalias */
#include "names.h"

struct VT {
	char pad[8];
	short off;
	short p;
	void (*fn)(char *, int);
};

typedef struct P {
	char p0[4];
	int f4;
	char pa[0x48 - 8];
	struct VT *vt;
	char p1[0x78e - 0x4c];
	short s78e;
	char p2[0x794 - 0x790];
	short s794;
	char p3[0x79a - 0x796];
	short s79a;
} P;

struct E {
	int f0, f4, f8;
	char rest[0x154 - 12];
};

extern struct E Stats[];
extern int DAT_800d2bf0[];
extern int GHeadPart;
extern int pDoubleBuffer;
extern char *MechList;
extern char MikeCamera[];

short DAT_800d2294[3];
int DAT_800d2284, DAT_800d2288, DAT_800d228c, DAT_800d1ba8;
P *DAT_800d229c;
short *DAT_800d22a0;

void FUN_80043f70(P *);
void FileIO_Continue(void);
P *FUN_800506d0(int);
P *FUN_8006b1c8(void);
void FUN_8006ed2c(int);
void FUN_8006edb8(int);
void ProcessNewPSX(int);
void ClearRegion(int, int, int);
int Spool_PSX(int, int, int);
void Spool_Continue(void);
int FUN_80070830(void);
void M3d_RenderSetup(char *, short *, char *);
void FUN_80083774(void);
void M3d_Render(char *);
void FUN_80093240(short *, P *);
void M3d_BuildTransform(P *);
void Spool_ClearPSX(int);
void Spool_Sync(void);

void FUN_800507e4(int a, int b) {
	short *q;

	switch (a) {
		case 0:
			DAT_800d2284 = -1;
			DAT_800d2288 = 0;
			DAT_800d228c = 500;
			DAT_800d2bf0[0] = Spool_PSX(Stats[0].f8, 2, 1);
			Spool_Sync();

			DAT_800d2294[2] = 0;
			DAT_800d2294[1] = 0;
			DAT_800d2294[0] = 0;
			q = (short *)FUN_8006b1c8();
			DAT_800d22a0 = q;
			q[0] = 512;
			q[1] = 240;
			DAT_800d229c = 0;
			break;
		case 1:
			FileIO_Continue();
			Spool_Continue();

			if (DAT_800d228c == 500 && FUN_80070830() == 0) {
				if (DAT_800d2284 >= 0) {
					if (DAT_800d229c)
						DAT_800d229c->vt->fn((char *)DAT_800d229c + DAT_800d229c->vt->off, 3);

					ClearRegion(DAT_800d2bf0[DAT_800d2284], 1, 1);
					FUN_8006edb8(DAT_800d2bf0[DAT_800d2284]);
					ProcessNewPSX(DAT_800d2bf0[DAT_800d2284]);
					Spool_ClearPSX(Stats[DAT_800d2284].f8);
				}

				FUN_8006ed2c(DAT_800d2bf0[DAT_800d2288]);
				ProcessNewPSX(DAT_800d2bf0[DAT_800d2288]);

				DAT_800d229c = FUN_800506d0(DAT_800d2288);
				DAT_800d229c->f4 = DAT_800d1ba8;
				GHeadPart = -1;
				DAT_800d2284 = DAT_800d2288;

				if (++DAT_800d2288 >= 10)
					DAT_800d2288 = 0;

				DAT_800d2bf0[DAT_800d2288] = Spool_PSX(Stats[DAT_800d2288].f8, 2, 1);
			}

			DAT_800d228c--;

			if (DAT_800d228c <= 0)
				DAT_800d228c = 500;

			FUN_80093240(DAT_800d2294, (P *)((char *)DAT_800d229c + 0x78c));

			{
				P *q2 = DAT_800d229c;
				unsigned short sa = q2->s78e, sb = q2->s794, sc = q2->s79a;

				q2->s78e = -sa;
				q2->s794 = -sb;
				q2->s79a = -sc;
				M3d_BuildTransform(q2);
			}

			FUN_80043f70(DAT_800d229c);

			if (b)
				FUN_80043f70(DAT_800d229c);

			M3d_RenderSetup(MikeCamera, DAT_800d22a0, (char *)pDoubleBuffer + 0x8c);
			M3d_Render(MechList);
			FUN_80083774();

			DAT_800d2294[1] += 10;
			break;
		case 2:
			if (DAT_800d229c)
				DAT_800d229c->vt->fn((char *)DAT_800d229c + DAT_800d229c->vt->off, 3);

			ClearRegion(DAT_800d2bf0[DAT_800d2284], 1, 1);
			FUN_8006edb8(DAT_800d2bf0[DAT_800d2284]);
			ProcessNewPSX(DAT_800d2bf0[DAT_800d2284]);
			Spool_ClearPSX(Stats[DAT_800d2284].f8);
			Spool_Sync();
			ProcessNewPSX(DAT_800d2bf0[DAT_800d2288]);
			Spool_ClearPSX(Stats[DAT_800d2288].f8);
			break;
	}
}

/* FUN_800507e4 (0x800507e4) flags=-fstructure-noalias */
/* C++ (front end): the rotating skater preview. 0 starts spooling the first skater's model, 1 (each
   frame) swaps in the next skater every 500 frames and draws the current one, 2 tears down. */
#include "game/bruce.h"
#include "game/camera.h"
#include "game/career.h"
#include "game/dbuffer.h"
#include "game/m3d.h"

extern SStats Stats[];
extern int DAT_800d2bf0[];
extern int GHeadPart;
extern CBruce *MechList[2];
extern SCamera MikeCamera[];

short DAT_800d2294[3];
int DAT_800d2284, DAT_800d2288, DAT_800d228c, DAT_800d1ba8;
CBruce *DAT_800d229c;
SViewport *DAT_800d22a0;

void FileIO_Continue();
extern "C" CBruce *FUN_800506d0(int);
SViewport *HiRes_SetupCamera();
void Spool_BackupPSX(int);
void Spool_RestorePSX(int);
void ProcessNewPSX(int);
void ClearRegion(int, int, int);
int Spool_PSX(char *, int, int);
void Spool_Continue();
extern "C" int FUN_80070830(void);
void M3d_BuildTransform(CSuper *);
void Spool_ClearPSX(char *);
void Spool_Sync();

extern "C" void FUN_800507e4(int a, int b) {
	SViewport *q;

	switch (a) {
		case 0:
			DAT_800d2284 = -1;
			DAT_800d2288 = 0;
			DAT_800d228c = 500;
			DAT_800d2bf0[0] = Spool_PSX(Stats[0].FrontName, 2, 1);
			Spool_Sync();

			DAT_800d2294[2] = 0;
			DAT_800d2294[1] = 0;
			DAT_800d2294[0] = 0;
			q = HiRes_SetupCamera();
			DAT_800d22a0 = q;
			q->xR = 512;
			q->yB = 240;
			DAT_800d229c = 0;
			break;
		case 1:
			FileIO_Continue();
			Spool_Continue();

			if (DAT_800d228c == 500 && FUN_80070830() == 0) {
				if (DAT_800d2284 >= 0) {
					if (DAT_800d229c)
						delete DAT_800d229c;

					ClearRegion(DAT_800d2bf0[DAT_800d2284], 1, 1);
					Spool_RestorePSX(DAT_800d2bf0[DAT_800d2284]);
					ProcessNewPSX(DAT_800d2bf0[DAT_800d2284]);
					Spool_ClearPSX(Stats[DAT_800d2284].FrontName);
				}

				Spool_BackupPSX(DAT_800d2bf0[DAT_800d2288]);
				ProcessNewPSX(DAT_800d2bf0[DAT_800d2288]);

				DAT_800d229c = FUN_800506d0(DAT_800d2288);
				DAT_800d229c->mPos.vx = DAT_800d1ba8;
				GHeadPart = -1;
				DAT_800d2284 = DAT_800d2288;

				if (++DAT_800d2288 >= 10)
					DAT_800d2288 = 0;

				DAT_800d2bf0[DAT_800d2288] = Spool_PSX(Stats[DAT_800d2288].FrontName, 2, 1);
			}

			DAT_800d228c--;

			if (DAT_800d228c <= 0)
				DAT_800d228c = 500;

			RotMatrixYXZ((SVECTOR *)DAT_800d2294, &DAT_800d229c->mRotMatrix);

			{
				CBruce *q2 = DAT_800d229c;
				unsigned short sa = q2->mRotMatrix.m[0][1], sb = q2->mRotMatrix.m[1][1],
				               sc = q2->mRotMatrix.m[2][1];

				q2->mRotMatrix.m[0][1] = -sa;
				q2->mRotMatrix.m[1][1] = -sb;
				q2->mRotMatrix.m[2][1] = -sc;
				M3d_BuildTransform(q2);
			}

			DAT_800d229c->UpdateFrame();

			if (b)
				DAT_800d229c->UpdateFrame();

			M3d_RenderSetup(MikeCamera, DAT_800d22a0, pDoubleBuffer->OT3D);
			M3d_Render(MechList[0]);
			M3d_RenderCleanup();

			DAT_800d2294[1] += 10;
			break;
		case 2:
			if (DAT_800d229c)
				delete DAT_800d229c;

			ClearRegion(DAT_800d2bf0[DAT_800d2284], 1, 1);
			Spool_RestorePSX(DAT_800d2bf0[DAT_800d2284]);
			ProcessNewPSX(DAT_800d2bf0[DAT_800d2284]);
			Spool_ClearPSX(Stats[DAT_800d2284].FrontName);
			Spool_Sync();
			ProcessNewPSX(DAT_800d2bf0[DAT_800d2288]);
			Spool_ClearPSX(Stats[DAT_800d2288].FrontName);
			break;
	}
}

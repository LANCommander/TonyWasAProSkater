/* Db_WaitRender__Fv (0x80048fec) flags=-fstrict-aliasing */
#include "names.h"
#ifdef PC_PORT
#include "pc_draw.h"
#endif

typedef struct {
	char pad0[0x74];
	int Busy;   /* 0x74 */
	int Flag78; /* 0x78 */
	char pad7c[0x84 - 0x7c];
	unsigned long OT[(0x80a8 - 0x84) / 4];
	unsigned long Prims; /* 0x80a8 */
} SBuffer;

SBuffer *pDoubleBuffer;
SBuffer *pVB;
unsigned long pPoly;

void Pause__Fi(int);
void DrawSync(int);
void Db_CallBack(void);
int Db_OTSize(void);
void FUN_8008c7d4(unsigned long *, int);

void Db_WaitRender(void) {
	pVB = pDoubleBuffer;

	while (pVB->Busy)
		Pause__Fi(1);

	if (pDoubleBuffer->Flag78) {
		DrawSync(0);
		Db_CallBack();
	}

#ifdef PC_PORT
	PC_DrawClearOTR((void *)(pDoubleBuffer->OT), Db_OTSize());
#else
	FUN_8008c7d4(pDoubleBuffer->OT, Db_OTSize());
#endif

#ifdef PC_PORT
	/* the PS1 drew through KUSEG (bit 31 clear); on PC main RAM is only at 0x80000000 */
	pPoly = pDoubleBuffer->Prims | 0x80000000;
#else
	pPoly = pDoubleBuffer->Prims & 0x7fffffff;
#endif
}

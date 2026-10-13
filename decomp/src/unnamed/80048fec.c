/* FUN_80048fec (0x80048fec) flags=-fstrict-aliasing */
#include "names.h"

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
void FUN_8008c468(int);
void Db_CallBack(void);
int FUN_80048c74(void);
void FUN_8008c7d4(unsigned long *, int);

void FUN_80048fec(void) {
	pVB = pDoubleBuffer;

	while (pVB->Busy)
		Pause__Fi(1);

	if (pDoubleBuffer->Flag78) {
		FUN_8008c468(0);
		Db_CallBack();
	}

	FUN_8008c7d4(pDoubleBuffer->OT, FUN_80048c74());

#ifdef PC_PORT
	/* the PS1 drew through KUSEG (bit 31 clear); on PC main RAM is only at 0x80000000 */
	pPoly = pDoubleBuffer->Prims | 0x80000000;
#else
	pPoly = pDoubleBuffer->Prims & 0x7fffffff;
#endif
}

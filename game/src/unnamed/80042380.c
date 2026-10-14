/* FUN_80042380 (0x80042380) */
#include "names.h"

extern char DAT_800a0534[];

char MechList[8];
int NumMechs;

void SFX_Stop(int a);
void Mem_Delete(int a);
void CBody_DeleteFrom(char *p, char *l);
void FUN_800427bc(char *p);
void FUN_8001dfec(char *p, int a);
void dt_6CSuper(char *p, int a);
void __dl__5CItemPv(char *p);

void FUN_80042380(char *p, int f) {
	*(char **)(p + 72) = DAT_800a0534;

	if (*(int *)(p + 1736)) {
		SFX_Stop(*(int *)(p + 1736));

		*(int *)(p + 1736) = 0;
	}

	if (*(int *)(p + 1740)) {
		SFX_Stop(*(int *)(p + 1740));

		*(int *)(p + 1740) = 0;
	}

	if (*(int *)(p + 348))
		Mem_Delete(*(int *)(p + 348));

	if (*(int *)(p + 352))
		Mem_Delete(*(int *)(p + 352));

	CBody_DeleteFrom(p, MechList);

	NumMechs--;
	FUN_800427bc(p);
	FUN_8001dfec(p + 2772, 2);
	FUN_8001dfec(p + 2696, 2);
	dt_6CSuper(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

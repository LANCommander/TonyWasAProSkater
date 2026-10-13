/* FUN_800620f0 (0x800620f0) */
#include "names.h"

extern char __vt_11SlicedImage[];
extern char DAT_800a21b0[];

void SlicedImage_unpack(char *p);
void Pack_Unpack(short a, short b);
void Mem_Delete(char *p);
void Free256Slot(unsigned short s);
void __dl__6CClassPv(char *p);

void FUN_800620f0(char *p, int f) {
	*(char **)p = __vt_11SlicedImage;

	if (*(int *)(p + 44)) {
		SlicedImage_unpack(p);
	} else if (*(short **)(p + 24)) {
		Pack_Unpack(**(short **)(p + 24), (*(short **)(p + 24))[1]);
		Mem_Delete(*(char **)(p + 24));
	}

	if (*(int *)(p + 36) == 8)
		Free256Slot(*(unsigned short *)(p + 40));

	if (!*(int *)(p + 48))
		Mem_Delete(*(char **)(p + 20));

	*(char **)p = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

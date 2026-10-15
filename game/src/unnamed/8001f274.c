/* FUN_8001f274 (0x8001f274) */
#include "names.h"

/* the object: vptr, an owned buffer and its owned flag */
typedef struct {
	char *vt;
	void *pData;
	int Owned;
} F;

extern char DAT_8009ef94[];
extern char DAT_8009efa4[];

void Mem_Delete(void *a);
void __dl__6CClassPv(void *p);

void FUN_8001f274(F *p, int f) {
	p->vt = DAT_8009ef94;

	if (p->Owned)
		Mem_Delete(p->pData);

	p->vt = DAT_8009efa4;

	if (f & 1)
		__dl__6CClassPv(p);
}

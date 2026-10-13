/* FUN_80041264 (0x80041264) */
#include "names.h"

typedef struct {
	char pad[8];
	short adj;
	short pad2;
	void (*fn)(char *, int);
} VT;

extern char __vt_16CScriptOnlyBaddy[];

char BaddyList[8];

void CBody_DeleteFrom(char *p, char *l);
void SFX_Stop(int a);
void Mem_Delete(int a);
void dt_6CBaddy(char *p, int a);
void __dl__5CItemPv(char *p);

void FUN_80041264(char *p, int f) {
	int i;

	*(char **)(p + 72) = __vt_16CScriptOnlyBaddy;
	CBody_DeleteFrom(p, BaddyList);

	if (*(int *)(p + 556))
		SFX_Stop(*(int *)(p + 556));

	if (*(char **)(p + 552)) {
		char *o = *(char **)(p + 552);
		VT *v = *(VT **)(o + 68);

		v->fn(o + v->adj, 3);
	}

	if (*(int *)(p + 548)) {
		for (i = 0; i < *(unsigned short *)(p + 544); i++) {
			char *o = *(char **)(*(int *)(p + 548) + (i << 4) + 12);

			if (o) {
				VT *v = *(VT **)(o + 68);

				v->fn(o + v->adj, 3);
			}
		}

		Mem_Delete(*(int *)(p + 548));
	}

	dt_6CBaddy(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

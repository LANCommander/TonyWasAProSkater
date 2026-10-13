/* FUN_800543d4 (0x800543d4) */
#include "names.h"

struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

typedef struct {
	struct VT *vt;
} O;

extern int Loading;
extern int DAT_800d33c4[];
extern O *DAT_800d36a4[][15];

O *DAT_800d22dc;
int DAT_800d22e4;

void FileIO_Sync(void);
void Mem_Delete(int);

void FUN_800543d4(void) {
	int i, j;
	O *o;

	if (Loading)
		FileIO_Sync();

	for (i = 0; i < 3; i++) {
		for (j = 0; j < DAT_800d33c4[i]; j++) {
			o = DAT_800d36a4[i][j];

			if (o) {
				o->vt->fn((char *)o + o->vt->off, 3);

				DAT_800d36a4[i][j] = 0;
			}
		}
	}

	if (DAT_800d22e4)
		Mem_Delete(DAT_800d22e4);

	o = DAT_800d22dc;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);
}

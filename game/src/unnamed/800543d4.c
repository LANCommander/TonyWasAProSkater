/* Front_FreeBoardPictures1P__Fv (0x800543d4) */
#include "names.h"

/* a CClass object: its vptr at +0 (deleted through its vtable, pc_vdelete) */
typedef struct {
	void *vt;
} O;

extern int Loading;
extern int DAT_800d33c4[];
extern O *DAT_800d36a4[][15];

extern O *DAT_800d22dc;
extern void *DAT_800d22e4;

void FileIO_Sync(void);
void Mem_Delete(void *);

void Front_FreeBoardPictures1P(void) {
	int i, j;
	O *o;

	if (Loading)
		FileIO_Sync();

	for (i = 0; i < 3; i++) {
		for (j = 0; j < DAT_800d33c4[i]; j++) {
			o = DAT_800d36a4[i][j];

			if (o) {
				pc_vdelete(o->vt, 0, o);

				DAT_800d36a4[i][j] = 0;
			}
		}
	}

	if (DAT_800d22e4)
		Mem_Delete(DAT_800d22e4);

	o = DAT_800d22dc;

	if (o)
		pc_vdelete(o->vt, 0, o);
}

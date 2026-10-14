/* Front_FreeBoardPictures2P__Fv (0x800514f0) */
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
extern int DAT_800d2c1c[];
extern O *DAT_800d31cc[][3][15];

int DAT_800d22c0[2];
O *DAT_800d22b0;

void FileIO_Sync(void);
void Mem_Delete(int);

void Front_FreeBoardPictures2P(void) {
	int k, i, j;
	O *o;

	if (Loading)
		FileIO_Sync();

	for (k = 0; k < 2; k++) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < DAT_800d2c1c[i]; j++) {
				o = DAT_800d31cc[k][i][j];

				if (o) {
					o->vt->fn((char *)o + o->vt->off, 3);

					DAT_800d31cc[k][i][j] = 0;
				}
			}
		}

		if (DAT_800d22c0[k])
			Mem_Delete(DAT_800d22c0[k]);
	}

	o = DAT_800d22b0;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);
}

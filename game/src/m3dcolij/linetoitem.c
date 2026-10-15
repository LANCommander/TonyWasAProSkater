/* M3dColij_LineToItem__FP4ItemP9SLineInfo (0x8007ee84) */
#include "names.h"
#include <sys/types.h>
#include <libgte.h>

typedef struct Item {
	short _0;
	unsigned short inq;
	char _4[0x18];
	struct Item *next;
} Item;

typedef struct {
	char _0[0x44];
	int flag;
	char mat[0x20];
	Item *pItem;
	int pos[3];
	SVECTOR normal;
	unsigned int *pFace;
	int model;
	unsigned char rec, drop;
	unsigned short inq;
} SLineInfo;

extern void FUN_8008a658(Item *, int, SLineInfo *, unsigned short);
extern void M3dColij_LineToThisItem(Item *, SLineInfo *);

void M3dColij_LineToItem(Item *it, SLineInfo *li) {
	if (it && li->flag) {
		geom_ld_mat(pc_geom.rot, li->mat);
		FUN_8008a658(it, 0, li, li->inq);

		do {
			if (it->inq != li->inq) {
				it->inq = li->inq;
				M3dColij_LineToThisItem(it, li);
			}

			it = it->next;
		} while (it);
	}
}

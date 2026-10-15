/* M3dColij_GetLineInfo__FP9SLineInfo (0x800808b0) */
#include <sys/types.h>
#include <libgte.h>

typedef struct {
	char _0[0x68];
	char *obj;
	char _6c[0x78 - 0x6c];
	short v[3];
} SLineInfo;

extern char Tfm[];
typedef struct {
	void *pItem, *pFace;
	SVECTOR *pNormal;
} SHitN; /* SHit */

extern SHitN NearestHit;

void M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX(char *, char *);

int M3dColij_GetLineInfo(SLineInfo *li) {
	if (li->obj) {
		M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX(li->obj + 16, Tfm);
		geom_ld_mat(pc_geom.rot, Tfm);
		{
			int16_t v[3];
			GeomAcc r;

			geom_ld_sv(v, NearestHit.pNormal);
			r = geom_mvmva(pc_geom.rot, 0, v[0], v[1], v[2], 12, 0, 0);
			geom_st_sv(li->v, r.ir);
		}
		return 1;
	}

	return 0;
}

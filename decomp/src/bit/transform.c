/* Transform__FRC7CVectorRi (0x80027920) */
#include <sys/types.h>
#include <libgte.h>
#ifndef PC_PORT
#include <inline_c.h>
#endif

typedef struct {
	int x, y, z;
} V;

typedef struct {
	int pad;
	int x, y, z;
	char rest[164 - 16];
} Cam;

extern Cam MikeCamera[];
extern int Camera_CurrentMikeCamera;

int Transform(const V *in, int *out) {
	V v;
	int z;

	v.x = (in->x >> 12) - MikeCamera[Camera_CurrentMikeCamera].x;
	v.y = (in->y >> 12) - MikeCamera[Camera_CurrentMikeCamera].y;
	v.z = (in->z >> 12) - MikeCamera[Camera_CurrentMikeCamera].z;
#ifdef PC_PORT
	{
		GeomProj p = geom_project_lv(&v);

		*out = (int)p.sxy;
		z = p.v.mac[2];
	}
#else
	gte_ldlv0(&v);
	gte_rtps();
	gte_stsxy(out);
	gte_stlvnl2(&z);
#endif
	return z;
}

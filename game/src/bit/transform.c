/* Transform__FRC7CVectorRi (0x80027920) */
#include <sys/types.h>
#include <libgte.h>

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
	{
		GeomProj p = geom_project_lv(&v);

		*out = (int)p.sxy;
		z = p.v.mac[2];
	}
	return z;
}

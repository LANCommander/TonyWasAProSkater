/* TransformVertex__FR7CVectorP7SVECTORPUci (0x800216d0) */
#include <sys/types.h>
#include <libgte.h>

typedef struct {
	int x, y, z;
} V;

extern V *__als__7CVectorRCi(V *, const int *);

void TransformVertex(V *out, SVECTOR *verts, unsigned char *idx, int i) {
	int s;

	geom_transform_sv(&verts[idx[i]], out);

	s = 12;
	__als__7CVectorRCi(out, &s);
}

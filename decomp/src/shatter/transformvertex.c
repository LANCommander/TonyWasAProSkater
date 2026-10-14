/* TransformVertex__FR7CVectorP7SVECTORPUci (0x800216d0) */
#include <sys/types.h>
#include <libgte.h>
#ifndef PC_PORT
#include <inline_c.h>
#endif

typedef struct {
	int x, y, z;
} V;

extern V *__als__7CVectorRCi(V *, const int *);

void TransformVertex(V *out, SVECTOR *verts, unsigned char *idx, int i) {
	int s;

#ifdef PC_PORT
	geom_transform_sv(&verts[idx[i]], out);
#else
	gte_ldv0(&verts[idx[i]]);
	gte_rtv0tr();
	gte_stlvnl(out);
#endif

	s = 12;
	__als__7CVectorRCi(out, &s);
}

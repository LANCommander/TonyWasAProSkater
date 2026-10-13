/* Move__11CGrindSpark (0x8002780c) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	char _0[4];
	V pos;
} Src;

typedef struct {
	char _0[8];
	unsigned short life;
	unsigned short maxlife;
	char _c[0xc];
	V p;
	V v;
	V a;
	char _3c[0x50 - 0x3c];
	V q0;
	V q1;
	V o;
	char _74[0];
	Src *src;
} GS;

void CBit_Die(GS *);

void CGrindSpark_Move(GS *t) {
	int px = t->p.x, py = t->p.y, pz = t->p.z;
	int vx = t->v.x, vy = t->v.y, vz = t->v.z;
	int nx = px + vx, ny = py + vy, nz = pz + vz;
	Src *s = t->src;
	int sx = s->pos.x, sy = s->pos.y, sz = s->pos.z;

	t->life++;
	t->o.x = px;
	t->o.y = py;
	t->o.z = pz;
	t->p.x = nx;
	t->p.y = ny;
	t->p.z = nz;
	t->q0.x = nx + sx;
	t->q0.y = ny + sy;
	t->q0.z = nz + sz;
	vx += t->a.x;
	vx -= vx >> 3;
	vy += t->a.y;
	vy -= vy >> 3;
	t->q1.x = px + sx;
	t->q1.y = py + sy;
	t->q1.z = pz + sz;
	t->v.x = vx;
	t->v.y = vy;
	vz += t->a.z;
	vz -= vz >> 3;
	t->v.z = vz;

	if ((short)t->life >= t->maxlife)
		CBit_Die(t);
}

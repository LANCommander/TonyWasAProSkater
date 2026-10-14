typedef struct {
	int x, y, z;
} V;

V op(const V *a, const V *b) {
	V r;

	r.x = a->x - b->x;
	r.y = a->y - b->y;
	r.z = a->z - b->z;
	return r;
}

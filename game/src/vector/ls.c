typedef struct {
	int x, y, z;
} V;

V op(const V *a, const int *n) {
	V r;

	r.x = a->x << *n;
	r.y = a->y << *n;
	r.z = a->z << *n;
	return r;
}

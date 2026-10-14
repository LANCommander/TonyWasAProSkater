typedef struct {
	int x, y, z;
} V;

V op(const int *n, const V *a) {
	V r;

	r.x = a->x * *n;
	r.y = a->y * *n;
	r.z = a->z * *n;
	return r;
}

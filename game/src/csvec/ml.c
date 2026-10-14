/* __ml__FRCiRC8CSVector (0x8004e11c) */
typedef struct {
	short x, y, z;
} S;

S op(const int *n, const S *a) {
	S r;

	r.x = a->x * *n;
	r.y = a->y * *n;
	r.z = a->z * *n;
	return r;
}

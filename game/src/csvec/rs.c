/* __rs__FRC8CSVectorRCi (0x8004e184) */
typedef struct {
	short x, y, z;
} S;

S op(const S *a, const int *n) {
	S r;

	r.x = a->x >> *n;
	r.y = a->y >> *n;
	r.z = a->z >> *n;
	return r;
}

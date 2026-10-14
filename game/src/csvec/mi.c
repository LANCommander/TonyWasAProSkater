/* __mi__FRC8CSVectorT0 (0x8004e0c4) */
typedef struct {
	short x, y, z;
} S;

S op(const S *a, const S *b) {
	S r;

	r.x = a->x - b->x;
	r.y = a->y - b->y;
	r.z = a->z - b->z;
	return r;
}

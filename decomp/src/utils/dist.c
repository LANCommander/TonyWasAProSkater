/* Utils_Dist__FRC7CVectorT0 (0x80047bf4) */
typedef struct {
	int x, y, z;
} V;

int M3dMaths_SquareRoot0__Fi(int);

int Utils_Dist(V *a, V *b) {
	V d;

	d.x = (a->x - b->x) >> 12;
	d.y = (a->y - b->y) >> 12;
	d.z = (a->z - b->z) >> 12;
	return M3dMaths_SquareRoot0__Fi(d.x * d.x + d.y * d.y + d.z * d.z);
}

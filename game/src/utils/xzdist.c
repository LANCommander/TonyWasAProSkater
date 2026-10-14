/* Utils_XZDist__FRC7CVectorT0 (0x80047c70) */
typedef struct {
	int x, y, z;
} V;

int M3dMaths_SquareRoot0__Fi(int);

int Utils_XZDist(V *a, V *b) {
	int dx = (a->x - b->x) >> 12;
	int dz = (a->z - b->z) >> 12;

	return M3dMaths_SquareRoot0__Fi(dx * dx + dz * dz);
}

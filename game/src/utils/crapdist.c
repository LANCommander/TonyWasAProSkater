/* Utils_CrapDist__FRC7CVectorT0 (0x80047cc4) */
typedef struct {
	int x, y, z;
} V;

int Utils_CrapDist(const V *a, const V *b) {
	V d;

	d.x = a->x - b->x;
	d.y = a->y - b->y;
	d.z = a->z - b->z;

	if (d.x < 0)
		d.x = -d.x;

	if (d.y < 0)
		d.y = -d.y;

	if (d.z < 0)
		d.z = -d.z;

	if (d.x < d.y) {
		if (d.y < d.z)
			return ((d.x >> 2) + (d.y >> 1) + d.z) >> 12;
		else if (d.z < d.x)
			return ((d.z >> 2) + (d.x >> 1) + d.y) >> 12;
		else
			return ((d.x >> 2) + (d.z >> 1) + d.y) >> 12;
	} else {
		if (d.x < d.z)
			return ((d.y >> 2) + (d.x >> 1) + d.z) >> 12;
		else if (d.z < d.y)
			return ((d.z >> 2) + (d.y >> 1) + d.x) >> 12;
		else
			return ((d.y >> 2) + (d.z >> 1) + d.x) >> 12;
	}
}

/* Utils_TurnTowards__FG8CSVectorR8CSVectorT1T0i (0x80047f40) */
typedef struct {
	short x, y, z;
} SV;

void Utils_TurnTowards(SV a, SV *o1, SV *o2, SV b, int speed) {
	SV d;

	d.x = b.x - a.x;
	d.y = b.y - a.y;

	if (d.x < -2048)
		d.x += 4096;

	if (d.x > 2048)
		d.x -= 4096;

	if (d.y < -2048)
		d.y += 4096;

	if (d.y > 2048)
		d.y -= 4096;

	if (d.x == 0 && d.y == 0) {
		o1->x = 0;
		o1->y = 0;
	} else {
		o2->x = (speed * d.x) >> 8;
		o2->y = (speed * d.y) >> 8;
	}
}

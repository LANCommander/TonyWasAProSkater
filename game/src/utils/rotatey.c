/* Utils_RotateY__FR7CVectorRC7CVectori (0x800479a8) */
typedef struct {
	int x, y, z;
} V;

typedef struct {
	short s, c;
} SC;

extern SC rcossin_tbl[];

void Utils_RotateY(V *out, V *in, int a) {
	SC *b;
	SC *t;

	a &= 4095;
	b = rcossin_tbl;
	t = b + a;
	out->x = ((in->x >> 3) * t->c + (in->z >> 3) * t->s) >> 9;
	out->y = in->y;
	out->z = ((in->z >> 3) * t->c - (in->x >> 3) * t->s) >> 9;
}

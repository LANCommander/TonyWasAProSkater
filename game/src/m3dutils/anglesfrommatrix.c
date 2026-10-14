extern int M3dMaths_SquareRoot0__Fi(int);
extern int ratan2(int, int);

void f(short *out, char *m) {
	int y = *(short *)(m + 4);
	int x = *(short *)(m + 0x10);
	short s1 = *(short *)(m + 6);
	short s2 = *(short *)(m + 8);
	short s3 = *(short *)(m + 10);
	int r = M3dMaths_SquareRoot0__Fi(y * y + x * x);

	out[0] = (short)ratan2(-(int)s3, r);
	out[1] = (short)ratan2(y, x);
	out[2] = (short)ratan2((int)s1, (int)s2);
}

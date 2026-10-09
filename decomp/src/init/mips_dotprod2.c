/* Mips_DotProd2 (0x800720ac) -- C interpretation of hand-written assembly (fixedmath.s) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_fixedmath.c. */

/* 2D dot product a*b + c*d of 20.12 fixed-point numbers, in 20.12. Like Mips_CVecDotProd: the
 * 64-bit sum wraps, and bits 12..43 of it are returned with no rounding or saturation. */
int Mips_DotProd2(int a, int b, int c, int d) {
	unsigned long long sum;

	sum = (unsigned long long)((long long)a * b);
	sum += (unsigned long long)((long long)c * d);
	return (int)(sum >> 12);
}

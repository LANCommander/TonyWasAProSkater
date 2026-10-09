/* Mips_DotProd2Sign (0x80072038) -- C interpretation of hand-written assembly (fixedmath.s) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_fixedmath.c. */

/* The sign of a*b + c*d: -1, 0 or 1. The sum is the full 64-bit one Mips_DotProd2 shifts down,
 * so small values still count (no 20.12 truncation); it wraps at 64 bits like the retail code,
 * which tests HI|LO for zero and the top bit of HI for the sign. */
int Mips_DotProd2Sign(int a, int b, int c, int d) {
	unsigned long long sum;

	sum = (unsigned long long)((long long)a * b);
	sum += (unsigned long long)((long long)c * d);
	if (sum == 0)
		return 0;
	return (long long)sum < 0 ? -1 : 1;
}

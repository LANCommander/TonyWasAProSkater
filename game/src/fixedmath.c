/* fixedmath.s as C: the hand-written fixed-point helpers (20.12 multiply, dot products).
   Not byte-matching by design; the port's interpretations of the retail assembly. */
struct CVector {
	int vx, vy, vz;
};

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

/* Mips_FixMul__Fii (0x80072090) -- C interpretation of hand-written assembly (fixedmath.s) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_fixedmath.c. */
/* Product of two 20.12 fixed-point numbers, in 20.12: bits 12..43 of the 64-bit product
 * ((lo >> 12) | (hi << 20) in the retail code), truncated to 32 bits with no rounding. */
int Mips_FixMul__Fii(int a, int b) {
	return (int)((unsigned long long)((long long)a * b) >> 12);
}

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

/* Mips_CVecDotProd__FP7CVectorT0 (0x800720ec) -- C interpretation of hand-written assembly (fixedmath.s) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_fixedmath.c. */
/* Dot product of two 20.12 fixed-point vectors, in 20.12.
 *
 * The retail code accumulates the three 64-bit products in HI:LO with a manual carry, then
 * returns bits 12..43 of the sum ((lo >> 12) | (hi << 20)). The sum wraps at 64 bits and the
 * result is truncated to 32, with no rounding or saturation; the unsigned accumulator below
 * reproduces both. */
int Mips_CVecDotProd__FP7CVectorT0(struct CVector *a, struct CVector *b) {
	unsigned long long sum;

	sum = (unsigned long long)((long long)a->vx * b->vx);
	sum += (unsigned long long)((long long)a->vy * b->vy);
	sum += (unsigned long long)((long long)a->vz * b->vz);
	return (int)(sum >> 12);
}

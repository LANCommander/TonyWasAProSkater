/* Mips_CVecDotProd__FP7CVectorT0 (0x800720ec) -- C interpretation of hand-written assembly (fixedmath.s) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_fixedmath.c. */

struct CVector {
	int vx, vy, vz;
};

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

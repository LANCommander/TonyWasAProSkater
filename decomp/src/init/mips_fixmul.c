/* Mips_FixMul__Fii (0x80072090) -- C interpretation of hand-written assembly (fixedmath.s) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_fixedmath.c. */

/* Product of two 20.12 fixed-point numbers, in 20.12: bits 12..43 of the 64-bit product
 * ((lo >> 12) | (hi << 20) in the retail code), truncated to 32 bits with no rounding. */
int Mips_FixMul__Fii(int a, int b) {
	return (int)((unsigned long long)((long long)a * b) >> 12);
}

/* M3dMaths_MulDiv64__Fiii (0x8008b62c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* a * b / c with a 64-bit intermediate, all unsigned (multu): the 64-bit product is divided by c
 * with a 32-step restoring shift-subtract division that keeps a 32-bit remainder. That is the
 * exact quotient when the product's high word is below c and c < 2^31; outside that range the
 * quotient bits come out as the loop below produces them (remainder bits shifted out are lost;
 * c == 0 gives 0xffffffff). */
int M3dMaths_MulDiv64__Fiii(int a, int b, int c)
{
	unsigned long long p = (unsigned long long)(unsigned)a * (unsigned)b;
	unsigned rem = (unsigned)(p >> 32), lo = (unsigned)p, q = 0;
	int i;

	for (i = 0; i < 32; i++) {
		rem = (rem << 1) | (lo >> 31);
		lo <<= 1;
		q <<= 1;
		if (rem >= (unsigned)c) {
			rem -= (unsigned)c;
			q |= 1;
		}
	}
	return (int)q;
}

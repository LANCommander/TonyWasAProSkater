/* M3dMaths_SquareRoot0__Fi (0x8008b5dc) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "m3dmaths.h"

/* Square-root table: entry i (1024 <= i < 4096) is the root of i << 20, for the top 12 bits of
 * a normalised input. Retail addresses it from 0x800c7490 (index 0); indices below 1024 are
 * never used, so the table proper starts at 0x800c7c90. */
extern unsigned short SquareRootTable[3072];

/* Integer square root by table lookup. The leading-zero count gives the shift that normalises
 * n by an even amount; the top 12 bits of the normalised value index the table and the result
 * is shifted back by half that amount. Negative n (no normalisation) looks up its top 12 bits
 * directly. */
int M3dMaths_SquareRoot0__Fi(int n) {
	unsigned shift, top12;


	if (n < 0)
		return SquareRootTable[((unsigned)n >> 20) - 1024];

	if (n == 0)
		return 0; /* retail reads index 0 and shifts it right by 16: always 0 */

	shift = (unsigned)geom_lzc(n) & ~1u;
	top12 = ((unsigned)n << shift) >> 20;
	return SquareRootTable[top12 - 1024] >> (shift / 2);
}

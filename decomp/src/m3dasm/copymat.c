/* M3dMaths_CopyMat__FPUsT0 (0x8008b68c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Copies a matrix's nine rotation halfwords from src to dst (the translation is not copied).
 * Retail reads all of src before writing dst. */
void M3dMaths_CopyMat__FPUsT0(unsigned short *src, unsigned short *dst) {
	unsigned short tmp[9];
	int i;

	for (i = 0; i < 9; i++)
		tmp[i] = src[i];

	for (i = 0; i < 9; i++)
		dst[i] = tmp[i];
}

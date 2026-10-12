/* M3dMaths_TransposeMat__FPUsT0 (0x8008b6bc) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Writes the transpose of src's 3x3 rotation (nine halfwords, row-major) to dst. Retail reads
 * all of src first, so src == dst transposes in place. */
void M3dMaths_TransposeMat__FPUsT0(unsigned short *src, unsigned short *dst)
{
	unsigned short tmp[3][3];
	int r, c;

	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			tmp[r][c] = src[r * 3 + c];
	for (r = 0; r < 3; r++)
		for (c = 0; c < 3; c++)
			dst[r * 3 + c] = tmp[c][r];
}

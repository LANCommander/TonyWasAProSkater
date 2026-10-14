/* M3dMaths_ZeroMat__FPUs (0x8008b670) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Clears a matrix's nine rotation halfwords (the translation is left alone). */
void M3dMaths_ZeroMat__FPUs(unsigned short *m) {
	int i;

	for (i = 0; i < 9; i++)
		m[i] = 0;
}

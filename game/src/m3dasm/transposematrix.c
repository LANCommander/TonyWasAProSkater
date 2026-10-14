/* M3dMaths_TransposeMatrix__FP6MATRIX (0x8008b5a4) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Transposes a matrix's 3x3 rotation in place (the translation is left alone). */
#include <sys/types.h>
#include <libgte.h>

void M3dMaths_TransposeMatrix__FP6MATRIX(MATRIX *m) {
	int i, j;

	for (i = 0; i < 3; i++)
		for (j = i + 1; j < 3; j++) {
			short t = m->m[i][j];

			m->m[i][j] = m->m[j][i];
			m->m[j][i] = t;
		}
}

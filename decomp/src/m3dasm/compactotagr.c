/* CompactOTagR (0x80089ddc) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include <stdint.h>
#ifdef PC_PORT
#include "pc_render3d.h"
#endif

/* An ordering-table link: a PS1 pointer's low 24 bits. (Retail also writes through such
 * stripped pointers, the KUSEG mirror of the same RAM; here the table's own pointers are used.) */
static uint32_t ot_link(const uint32_t *p) {
	return (uint32_t)p & 0xffffff;
}

/* Shortens the links of a reverse ordering table (ClearOTagR layout: ot[i] links to ot[i - 1],
 * drawn from ot[n - 1] down to ot[0]) once the primitives are in. In each run of empty entries
 * (an entry still linking to the next one down) the first entry, which the chain above leads
 * into, gets the link of the next entry that has primitives, so the run is skipped; ot[0] (the
 * terminator) ends the last run. The GPU then walks only the entries that draw something. */
void CompactOTagR(uint32_t *ot, int n) {
	uint32_t *link = &ot[n - 1]; /* the entry whose link is being extended */
	int skipping = 1;            /* inside a run of empty entries that starts at *link */
	int i;

#ifdef R3D_HOOKS
	/* the native renderer's layers find each bucket by walking every entry */
	if (R3d_Native())
		return;
#endif

	for (i = n - 1; i > 0; i--) {
		int empty = ot[i] == ot_link(&ot[i - 1]);

		if (skipping && !empty) {
			*link = ot[i];
			skipping = 0;
		} else if (!skipping && empty) {
			link = &ot[i];
			skipping = 1;
		}
	}

	if (skipping)
		*link = ot[0];
}

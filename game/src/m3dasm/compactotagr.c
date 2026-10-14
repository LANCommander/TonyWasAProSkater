/* CompactOTagR (0x80089ddc) -- C interpretation of hand-written assembly (M3DASM) */
#include <sys/types.h>
#include <libgpu.h>
#include "pc_render3d.h"

/* Shortens the links of a reverse ordering table (ClearOTagR layout: ot[i] links to ot[i - 1],
 * drawn from ot[n - 1] down to ot[0]) once the primitives are in. In each run of empty entries
 * (an entry still linking to the next one down) the first entry, which the chain above leads
 * into, gets the link of the next entry that has primitives, so the run is skipped; ot[0] (the
 * terminator) ends the last run. The GPU then walks only the entries that draw something. */
void CompactOTagR(P_TAG *ot, int n) {
	P_TAG *link = &ot[n - 1]; /* the entry whose link is being extended */
	int skipping = 1;         /* inside a run of empty entries that starts at *link */
	int i;

	/* the native renderer's layers find each bucket by walking every entry */
	if (R3d_Native())
		return;

	for (i = n - 1; i > 0; i--) {
		int empty = ot[i].next == &ot[i - 1];

		if (skipping && !empty) {
			link->next = ot[i].next;
			skipping = 0;
		} else if (!skipping && empty) {
			link = &ot[i];
			skipping = 1;
		}
	}

	if (skipping)
		link->next = ot[0].next;
}

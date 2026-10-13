/* M3dAsm_LineColijPreprocessItems__FP4ItemiP9SLineInfoUs (0x8008a658) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Coarse pass of a line-collision query over a linked item list (M3dColij_LineToItem): every item
 * whose model's bounding box the line can't touch is marked as already looked at (its Inquiry set
 * to this query's), so the per-face test skips it. Items that don't collide at all get the same
 * treatment.
 *
 * The test works in the item's model space: the line is made relative to the item, and for a
 * rotated item first checked against the bounding cube of the model's radius, then rotated by the
 * transposed item matrix (built in the scratchpad) and re-sorted. A scaled item's box is scaled.
 *
 * Retail quirk kept: the axis-swap flags of a rotated item's re-sorted line are toggled onto the
 * running flags and never reset, so every later unrotated item mirrors its box by them as well.
 *
 * test_asm: ignore 0x1f800000 0x1f800004 0x1f800008 0x1f80000c 0x1f800010 0x1f800014 0x1f800024
 * (the retail code parks its caller's s0-s5 and its return address there and restores them; a C
 * caller has no such registers, and nothing reads the slots) */
#include "linecolij.h"

extern SPSXRegion PSXRegion[];

void M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX(SVECTOR *angles, MATRIX *m);
void M3dMaths_TransposeMatrix__FP6MATRIX(MATRIX *m);

/* scratchpad words the retail code leaves behind */
#define SCR_MODEL   (*(SModel **)0x1f800018) /* last rotated item's model, item, &PSXRegion */
#define SCR_ITEM    (*(Item **)0x1f80001c)
#define SCR_REGIONS (*(SPSXRegion **)0x1f800020)
#define SCR_MATRIX  ((MATRIX *)0x1f800028)  /* the item rotation, transposed */
#define SCR_LINE    ((LineSeg *)0x1f80003c) /* the sorted query line, world model units */

/* rotate one point by the GTE rotation matrix (IR = RT * IR >> 12, saturated) */
static void Rotate(int *x, int *y, int *z) {
	PC_GteSetData(9, (unsigned)*x);
	PC_GteSetData(10, (unsigned)*y);
	PC_GteSetData(11, (unsigned)*z);
	gte_mvmva(1, 0, 3, 3, 0);

	*x = GTE_S(9);
	*y = GTE_S(10);
	*z = GTE_S(11);
}

void M3dAsm_LineColijPreprocessItems__FP4ItemiP9SLineInfoUs(Item *item, int unused, SLineInfo *info,
                                                            unsigned short inquiry) {
	unsigned flip;

	if (!item)
		return;

	GetLineSegment(info, SCR_LINE, &flip);

	do {
		SModel *model;
		LineSeg l;
		LineBox box;
		static const int zero[3];
		int i;

		if (ItemNotCollidable(item->Flags))
			goto reject;

		if (item->Inquiry == inquiry)
			goto next;

		model = PSXRegion[item->Region].ppModels[item->Model];

		/* the line relative to the item */
		l = *SCR_LINE;

		for (i = 0; i < 3; i++) {
			l.lo[i] -= item->Position[i] >> 12;
			l.hi[i] -= item->Position[i] >> 12;
		}

		if (*(int *)&item->Angles[0] || item->Angles[2]) {
			int r = model->Radius >> 12;
			unsigned *m;

			for (i = 0; i < 3; i++)
				if (l.lo[i] - r > 0 || l.hi[i] + r < 0)
					goto reject;

			SCR_MODEL = model;
			SCR_ITEM = item;
			SCR_REGIONS = PSXRegion;
			M3dMaths_RotMatrixYXZ__FP7SVECTORP6MATRIX((SVECTOR *)item->Angles, SCR_MATRIX);
			M3dMaths_TransposeMatrix__FP6MATRIX(SCR_MATRIX);

			m = (unsigned *)SCR_MATRIX;

			for (i = 0; i < 5; i++)
				PC_GteSetCtrl(i, m[i]);

			/* into model space: lo and hi are the line's (sorted) end points */
			Rotate(&l.lo[0], &l.lo[1], &l.lo[2]);
			Rotate(&l.hi[0], &l.hi[1], &l.hi[2]);
			SortLineSegmentCoords(&l, &flip);
		}

		for (i = 0; i < 3; i++)
			PC_GteSetCtrl(2 * i, (unsigned)(l.hi[i] - l.lo[i]));

		GetBoundingBox(model, zero, flip, &l, &box);

		if (item->Flags & ITEM_SCALED)
			for (i = 0; i < 3; i++) {
				/* low 32 bits of the product, as the retail mult/mflo */
				box.min[i] = (int)((unsigned)item->Scale[i] * (unsigned)box.min[i]) >> 12;
				box.max[i] = (int)((unsigned)item->Scale[i] * (unsigned)box.max[i]) >> 12;
			}

		if (LineToBoxTest(&l, &box))
			goto next;
	reject:
		item->Inquiry = inquiry;
	next:
		item = item->NextItem;
	} while (item);
}

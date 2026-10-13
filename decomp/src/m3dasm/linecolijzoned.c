/* M3dAsm_LineColijPreprocessItemsZoned__FPP4ItemiP9SLineInfoUs (0x8008a948) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* Coarse pass of a line-collision query over a zone's zero-terminated item array
 * (M3dColij_LineToItemZoned): every item whose model's bounding box, placed at the item's position,
 * the line can't touch is marked as already looked at (its Inquiry set to this query's), as are
 * items that don't collide at all. Rotation and scale are ignored here, and every item's model is
 * looked up in the region of the first item.
 *
 * test_asm: ignore 0x1f800000 0x1f800004 0x1f800008 0x1f80000c 0x1f800010 0x1f800014 0x1f800018
 * (the retail code parks its caller's s0-s6 there and restores them; a C caller has no such
 * registers, and nothing reads the slots) */
#include "linecolij.h"

extern SPSXRegion PSXRegion[];

void M3dAsm_LineColijPreprocessItemsZoned__FPP4ItemiP9SLineInfoUs(Item **items, int unused,
                                                                  SLineInfo *info,
                                                                  unsigned short inquiry) {
	Item *item = *items;
	SModel **models;
	LineSeg l;
	unsigned flip;

	if (!item)
		return;

	GetLineSegment(info, &l, &flip);

	models = PSXRegion[item->Region].ppModels;

	do {
		if (ItemNotCollidable(item->Flags))
			goto reject;

		if (item->Inquiry == inquiry)
			goto next;

		{
			int pos[3], i;
			LineBox box;

			for (i = 0; i < 3; i++)
				pos[i] = item->Position[i] >> 12;

			GetBoundingBox(models[item->Model], pos, flip, &l, &box);

			if (LineToBoxTest(&l, &box))
				goto next;
		}
	reject:
		item->Inquiry = inquiry;
	next:
		item = *++items;
	} while (item);
}

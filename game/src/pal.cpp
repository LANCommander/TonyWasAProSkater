/* PAL.CPP: palette (CLUT) loading: the palette entry list and the 16- and 256-colour CLUT slots
   in VRAM (declarations in game/pal.h). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/pal.h"
#include "game/m3d.h"
#include "pc_texstore.h"

#include "game/mem.h"
#include "game/pack.h"

extern tag_S_Pal *DAT_800d1a80; /* the newest palette entry */
extern char Free16Slots[];
extern char Free256Slots[];
tag_S_Pal *NewPaletteEntry(uint);

/* Pal_Init__Fv (0x8004aa6c) */
void Pal_Init(void) {
	int i;
	volatile int pad[2];

	DAT_800d1a80 = 0;
	Pack_Pack(-1, 256, 75, &Pal256X, &Pal256Y, 16, 0, 0);
	Pack_Pack(-1, 16, 256, &Pal16X, &Pal16Y, 16, 0, 0);

	for (i = 0; i < 256; i++)
		Free16Slots[i] = 1;

	Free256Slots[0] = 0;

	for (i = 1; i < 75; i++)
		Free256Slots[i] = 1;
}

/* NewPaletteEntry__FUi (0x8004ab2c) */
tag_S_Pal *NewPaletteEntry(uint checksum) {
	tag_S_Pal *n = (tag_S_Pal *)Mem_NewX(sizeof(tag_S_Pal), 0, 1);
	tag_S_Pal *h = DAT_800d1a80;

	n->pNext = 0;
	DAT_800d1a80 = n;
	n->pPrevious = h;

	if (h)
		h->pNext = n;

	n->Checksum = checksum;
	n->InVRAM = 0;
	n->Usage = 0;
	return n;
}

/* RemovePaletteEntry__FP9tag_S_Pal (0x8004ab88) */
/* a palette entry (see NewPaletteEntry) */
void RemovePaletteEntry(tag_S_Pal *n) {
	tag_S_Pal *p = n->pPrevious;
	tag_S_Pal *q;

	if (p)
		p->pNext = n->pNext;

	q = n->pNext;

	if (q)
		q->pPrevious = n->pPrevious;

	if (n == DAT_800d1a80)
		DAT_800d1a80 = n->pPrevious;

	Mem_Delete(n);
}

/* Pal_FindPaletteEntry__FUi (0x8004abf4) */
tag_S_Pal *Pal_FindPaletteEntry(uint id) {
	tag_S_Pal *p = DAT_800d1a80;

	while (p && p->Checksum != id)
		p = p->pPrevious; /* retail walks the list through +0x10 */

	return p;
}

/* Pal_RemoveUnusedPalettes__Fv (0x8004ac2c) */
void Pal_RemoveUnusedPalettes(void) {
	tag_S_Pal *p = DAT_800d1a80;

	while (p) {
		tag_S_Pal *next = p->pPrevious;

		if (!p->Usage) {
			if (p->flags & 1)
				Free16Slots[p->slot] = 1;

			if (p->flags & 2)
				Free256Slots[p->slot] = 1;

			RemovePaletteEntry(p);
		}

		p = next;
	}
}

/* GetFree16Slot__Fv (0x8004ace8) */
int GetFree16Slot(void) {
	int i = 0;

	while (!Free16Slots[i]) {
		i++;

		if (i >= 256)
			break;
	}

	Free16Slots[i] = 0;
	return i;
}

/* Free16Slot__FUs (0x8004ad38) */
void Free16Slot(unsigned short clut) {
	int i;

	for (i = 0; i < 256; i++)
		if (GetClut(Pal16X, Pal16Y + i) == clut)
			break;

	Free16Slots[i] = 1;
}

/* GetFree256Slot__Fv (0x8004ad9c) */
int GetFree256Slot(void) {
	int i = 0;

	while (!Free256Slots[i]) {
		i++;

		if (i >= 75)
			break;
	}

	Free256Slots[i] = 0;
	return i;
}

/* Free256Slot__FUs (0x8004adec) */
void Free256Slot(unsigned short clut) {
	int i;

	for (i = 0; i < 75; i++)
		if (GetClut(Pal256X, Pal256Y + i) == clut)
			break;

	Free256Slots[i] = 1;
}

/* Get256SlotNumber__FUs (0x8004ae50) */
int Get256SlotNumber(unsigned short clut) {
	int i;

	for (i = 0; i < 75; i++)
		if (GetClut(Pal256X, Pal256Y + i) == clut)
			break;

	return i;
}

/* Pal_LoadPalette__FUiPUiUc (0x8004aea4) */
/* Demo twin (retail drops the Remap path); locals from its MAIN.SYM. Allocates a
   palette entry, makes 0x7c1f (magenta) the transparent colour and sets the semi-transparency bit
   on the colours after it (or on all of them if there is none), reserves a 16- or 256-colour CLUT
   slot, uploads the palette and records its CLUT id. The tests re-read *pEntry rather than use a
   local: CSE serves them from the first load, which gives the andi 0xffff before the zero test. */
tag_S_Pal *Pal_LoadPalette(uint checksum, uint *pData, uchar flags) {
	tag_S_Pal *pNew;
	ushort *pEntry;
	RECT rec;
	int entries;
	int transparent_index;

	pNew = NewPaletteEntry(checksum);
	transparent_index = 0;
	pEntry = (ushort *)pData;
	entries = 256;
	pNew->flags = flags;
	rec.h = 1;

	if (flags & 1)
		entries = 16;

	{
		int i;

		for (i = 0; i < entries; i++) {
			if ((*pEntry & 0x7fff) == 0x7c1f) {
				transparent_index = 1;
				*pEntry = 0;
			} else if (transparent_index || !*pEntry) {
				*pEntry |= 0x8000;
			}

			pEntry++;
		}
	}

	if (!transparent_index) {
		int i;

		pEntry = (ushort *)pData;

		for (i = 0; i < entries; i++) {
			*pEntry |= 0x8000;
			pEntry++;
		}
	}

	if (entries == 16) {
		pNew->slot = GetFree16Slot();
		rec.x = Pal16X;
		rec.y = Pal16Y + pNew->slot;
		pNew->Clut = getClut(Pal16X, Pal16Y + pNew->slot);
	} else {
		pNew->slot = GetFree256Slot();
		rec.x = Pal256X;
		rec.y = Pal256Y + pNew->slot;
		pNew->Clut = getClut(Pal256X, Pal256Y + pNew->slot);
	}

	rec.w = entries;
	PC_TexUpload(rec.x, rec.y, rec.w, rec.h, pData);

	pNew->InVRAM = 1;
	return pNew;
}

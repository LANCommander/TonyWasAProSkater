/* Pal_LoadPalette__FUiPUiUc (0x8004aea4) */
/* C++ (PAL.CPP). Demo twin (retail drops the Remap path); locals from its MAIN.SYM. Allocates a
   palette entry, makes 0x7c1f (magenta) the transparent colour and sets the semi-transparency bit
   on the colours after it (or on all of them if there is none), reserves a 16- or 256-colour CLUT
   slot, uploads the palette and records its CLUT id. The tests re-read *pEntry rather than use a
   local: CSE serves them from the first load, which gives the andi 0xffff before the zero test. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

int Pal16X;
int Pal16Y;
int Pal256X;
int Pal256Y;

tag_S_Pal *Pal_NewEntry(uint);
int GetFree16Slot(void);
int GetFree256Slot(void);

tag_S_Pal *Pal_LoadPalette(uint checksum, uint *pData, uchar flags) {
	tag_S_Pal *pNew;
	ushort *pEntry;
	RECT rec;
	int entries;
	int transparent_index;

	pNew = Pal_NewEntry(checksum);
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
	LoadImage(&rec, (u_long *)pData);

	pNew->InVRAM = 1;
	return pNew;
}

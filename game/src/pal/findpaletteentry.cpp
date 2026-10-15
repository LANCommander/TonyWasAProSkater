/* Pal_FindPaletteEntry__FUi (0x8004abf4) */
#include "game/m3d.h"

tag_S_Pal *DAT_800d1a80;

tag_S_Pal *Pal_FindPaletteEntry(uint id) {
	tag_S_Pal *p = DAT_800d1a80;

	while (p && p->Checksum != id)
		p = p->pPrevious; /* retail walks the list through +0x10 */

	return p;
}

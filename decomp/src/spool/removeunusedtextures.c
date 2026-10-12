/* Spool_RemoveUnusedTextures__Fv (0x8006f1f8) */
#include "names.h"
typedef struct {
	char pad[4];
	unsigned short refs;
} SPalette;

typedef struct STexture {
	char pad[0x10];
	union {
		unsigned flags;
		unsigned short usage;
	} u;
	int f14;
	SPalette *pal;
	short px, py;
} STexture;

void GotoStartOfTextureList(void);
STexture *NextTexture(void);
void Pack_Unpack(short, short);
void RemoveTextureEntry(STexture *);

#ifdef PC_PORT
#include "pc_dctex.h"
#endif

void Spool_RemoveUnusedTextures(void) {
	STexture *t;

	GotoStartOfTextureList();

	while ((t = NextTexture())) {
#ifdef PC_PORT
		/* a Dreamcast texture (pc_dctex.h): no palette or VRAM area; clut = handle at 2, tpage
		   at 6 */
		if ((t->u.flags & 0xf0000) && !t->u.usage &&
		    PC_HD_IS_TPAGE(*(unsigned short *)((char *)t + 6))) {
			PC_HdTexRef(*(unsigned short *)((char *)t + 2), -1);
			RemoveTextureEntry(t);
			continue;
		}
#endif
		if ((t->u.flags & 0xf0000) && !t->u.usage) {
			t->pal->refs--;
			Pack_Unpack(t->px, t->py);
			RemoveTextureEntry(t);
		}
	}
}

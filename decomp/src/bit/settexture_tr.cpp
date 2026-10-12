/* SetTexture__15CTexturedRibbonUi (0x8002e2f0) */
/* C++ (BIT.CPP). Looks up the ribbon's texture by checksum. */
#include "game/bit.h"

Texture *Spool_FindTextureEntry(unsigned int);

void CTexturedRibbon::SetTexture(unsigned int checksum) {
	mpTex = Spool_FindTextureEntry(checksum);
}

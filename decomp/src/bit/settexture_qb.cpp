/* SetTexture__8CQuadBitPci (0x80028440) */
/* C++ (BIT.CPP). Uses frame Frame of a named spooled animation as the texture. */
#include "game/bit.h"
#include "game/m3d.h"

SAnimFrame *Spool_FindAnim(char *);

void CQuadBit::SetTexture(char *pName, int Frame) {
	mpTexture = Spool_FindAnim(pName)[Frame].pTexture;
}

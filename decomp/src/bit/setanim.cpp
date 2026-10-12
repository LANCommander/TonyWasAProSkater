/* SetAnim__7CFT4BitPc (0x80028100) */
/* C++ (BIT.CPP). Looks up a named spooled animation and starts it from frame 0 (the frame count is the byte
   4 bytes before the frame array). */
#include "game/bit.h"
#include "game/m3d.h"

SAnimFrame *Spool_FindAnim(char *);

void CFT4Bit::SetAnim(char *pName) {
	SAnimFrame *pAnim = Spool_FindAnim(pName);

	mpPSXAnim = pAnim;
	mFrame = 0;
	mFrameFrac = 0;
	mpPSXFrame = pAnim;
	mNumFrames = ((char *)pAnim)[-4];
}

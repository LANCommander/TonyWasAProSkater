/* GameFMV_StartFrameBufferCopy__Fv (0x80023260) */
#include "names.h"

extern int GameFMV_FrameBufferCopyActive;

void GameFMV_StartFrameBufferCopy(void) {
	GameFMV_FrameBufferCopyActive = 1;
}

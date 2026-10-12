/* _._6CSuper (0x80043d5c) */
/* C++ (OB.CPP). Frees the animation buffers the super item allocated; CBody's destructor does the
   rest. */
#include "game/ob.h"

void Mem_Delete(void *);

CSuper::~CSuper() {
	if (mpPoseBuffer)
		Mem_Delete(mpPoseBuffer);

	if (mpJoints)
		Mem_Delete(mpJoints);

	if (mpDecompressedFrame)
		Mem_Delete(mpDecompressedFrame);

	if (mpCalculationOrder)
		Mem_Delete(mpCalculationOrder);
}

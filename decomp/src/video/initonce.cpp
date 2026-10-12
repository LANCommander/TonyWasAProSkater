/* Video_InitOnce__Fv (0x800694a0) flags=-fstructure-noalias */
/* C++ (VIDEO.CPP). Demo twin (THPS1 allocates bigger replay buffers and skips a header); locals from
   its MAIN.SYM. Allocates player 1's replay buffer, and player 2's in a two-player game. */
#include "game/video.h"

extern SVideoInfo GVInfo1;
extern SVideoInfo GVInfo2;
extern int GNumberOfPlayers;

extern "C" char *Mem_NewX__FUiii(int, int, int);

void Video_InitOnce() {
	GVInfo1.pBlockStart = GVInfo1.pStreamStart = (ulong *)Mem_NewX__FUiii(24576, 1, 1);
	GVInfo1.pStreamStart += 576 / 4;

	if (GNumberOfPlayers == 2) {
		GVInfo2.pBlockStart = GVInfo2.pStreamStart = (ulong *)Mem_NewX__FUiii(24576, 1, 1);
		GVInfo2.pStreamStart += 576 / 4;
	} else {
		GVInfo2.pBlockStart = GVInfo2.pStreamStart = 0;
	}
}

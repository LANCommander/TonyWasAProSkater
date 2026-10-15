/* Video_InitOnce__Fv (0x800694a0) flags=-fstructure-noalias */
/* C++ (VIDEO.CPP). Demo twin (THPS1 allocates bigger replay buffers and skips a header); locals from
   its MAIN.SYM. Allocates player 1's replay buffer, and player 2's in a two-player game. */
#include "game/mem.h"
#include "game/video.h"

extern SVideoInfo GVInfo1[2];
extern int GNumberOfPlayers;

void Video_InitOnce() {
	GVInfo1[0].pBlockStart = GVInfo1[0].pStreamStart = (ulong *)Mem_NewX(24576, 1, 1);
	GVInfo1[0].pStreamStart += 576 / 4;

	if (GNumberOfPlayers == 2) {
		GVInfo1[1].pBlockStart = GVInfo1[1].pStreamStart = (ulong *)Mem_NewX(24576, 1, 1);
		GVInfo1[1].pStreamStart += 576 / 4;
	} else {
		GVInfo1[1].pBlockStart = GVInfo1[1].pStreamStart = 0;
	}
}

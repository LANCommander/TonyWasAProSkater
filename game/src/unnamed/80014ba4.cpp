/* Card_LoadDemoReplay__FPCc (0x80014ba4) */
#include "game/fileio.h"
#include "game/mem.h"
#include "game/video.h"

extern int GVideoRestart[];
extern SVideoInfo GVInfo1;

short Card_LoadDemoReplay(const char *name) {
	SVideoHead *h;
	char *b;
	int i;

	b = (char *)Mem_NewX(FileIO_Open(name), 1, 1);

	if (b) {
		FileIO_Load(b);
		FileIO_Sync();
	}

	h = (SVideoHead *)(b + 512);
	GVideoRestart[0] = h->NumSkaters;
	GVideoRestart[1] = h->Skater;
	GVideoRestart[2] = h->Skater2;
	GVideoRestart[3] = h->Level;
	GVideoRestart[4] = h->GGame;
	GVInfo1.NumFrames = h->NumFrames;

	for (i = 0; i < 5; i++)
		GVInfo1.Highlights[i] = h->Highlights[i];

	Mem_Delete(b);
	return h->NumSkaters;
}

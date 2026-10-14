/* strNext__Fv (0x80023e88) */
#include "pc_stream.h"
int GameFMV_CurrentFrame, GameFMV_EndFrame, GameFMV_RewindSwitch;

int FUN_800911fc(int *, int **);

int strNext(void) {
	int addr;
	int *hdr;

	if (!PC_StreamNextFrame((uint32_t **)&addr, (uint32_t **)&hdr))
		return 0;

	GameFMV_CurrentFrame = hdr[2];

	if (GameFMV_CurrentFrame >= GameFMV_EndFrame)
		GameFMV_RewindSwitch = 1;

	return addr;
}

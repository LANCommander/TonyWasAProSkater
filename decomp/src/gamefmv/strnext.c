/* strNext__Fv (0x80023e88) */
#ifdef PC_PORT
#include "pc_stream.h"
#endif
int GameFMV_CurrentFrame, GameFMV_EndFrame, GameFMV_RewindSwitch;

int FUN_800911fc(int *, int **);

int strNext(void) {
	int addr;
	int *hdr;

#ifdef PC_PORT
	if (!PC_StreamNextFrame((uint32_t **)&addr, (uint32_t **)&hdr))
		return 0;
#else
	if (FUN_800911fc(&addr, &hdr))
		return 0;
#endif

	GameFMV_CurrentFrame = hdr[2];

	if (GameFMV_CurrentFrame >= GameFMV_EndFrame)
		GameFMV_RewindSwitch = 1;

	return addr;
}

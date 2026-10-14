/* Front_StopMusicLoop__Fv (0x8005c85c) */
#include "names.h"
#include "pc_frame.h"

short DAT_800d1ca0;

void Front_SetMusicLoop(int a);
void Front_UpdateMusicLoop(void);
int VSync(int a);

void Front_StopMusicLoop(void) {
	Front_SetMusicLoop(-1);

	while (DAT_800d1ca0 >= 0) {
		Front_UpdateMusicLoop();
		PC_FrameWait(1);
	}
}

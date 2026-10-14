/* FlickerSync__Fv (0x80059018) */
#include "names.h"
#include "pc_frame.h"

int DAT_800d1c44;

void VSync(int a);
int FUN_8008cf04(void);

void FlickerSync(void) {
	PC_FrameWait(1);

	/* the interlace field (libgpu's GetODE): odd and even frames */
	for (;;) {
		PC_FramePoll();

		if ((int)(PC_FrameCount() & 1) != DAT_800d1c44)
			break;
	}

	DAT_800d1c44 ^= 1;
}

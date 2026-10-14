/* FlickerSync__Fv (0x80059018) */
#include "names.h"
#ifdef PC_PORT
#include "pc_frame.h"
#endif

int DAT_800d1c44;

void VSync(int a);
int FUN_8008cf04(void);

void FlickerSync(void) {
#ifdef PC_PORT
	PC_FrameWait(1);
#else
	VSync(0);
#endif

#ifdef PC_PORT
	/* the interlace field (libgpu's GetODE): odd and even frames */
	for (;;) {
		PC_FramePoll();

		if ((int)(PC_FrameCount() & 1) != DAT_800d1c44)
			break;
	}
#else
	while (FUN_8008cf04() == DAT_800d1c44)
		;
#endif

	DAT_800d1c44 ^= 1;
}

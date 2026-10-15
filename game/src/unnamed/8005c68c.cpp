/* Front_CheckSoftReset__Fv (0x8005c68c) */
/* Hold START+SELECT on either pad to count DAT_800d2348 down from 110 (eating the START trigger);
   at zero stop the looping sound and set DAT_800d1cb8. Called from 9 sites. */
#include "game/pad.h"

extern SControl Pad[2]; /* Pad */

extern int DAT_800d2348;      /* quit countdown */
extern unsigned DAT_800d1ca4; /* looping sound */
extern short DAT_800d1ca0;
extern int DAT_800d1cb8;

void SFX_Stop(unsigned);

void Front_CheckSoftReset(void) {
	if (!DAT_800d2348) {
		SFX_Stop(DAT_800d1ca4);

		DAT_800d1ca0 = 0;
		DAT_800d1cb8 = 1;
	} else if (Pad[0].Pause.Pressed && Pad[0].Camera.Pressed) {
		DAT_800d2348--;
		Pad[0].Pause.Triggered = 0;
	} else if (Pad[1].Pause.Pressed && Pad[1].Camera.Pressed) {
		DAT_800d2348--;
		Pad[1].Pause.Triggered = 0;
	} else {
		DAT_800d2348 = 110;
	}
}

/* Video_HandleStepping__Fv (0x8006ac84) */
#include "game/pad.h"
#include "game/video.h"

extern SControl Pad[2];
extern SVideoInfo GVInfo1;

extern int DemoTimer, Status, GNumberOfPlayers, GameFrozen, MusicStopped;

int DAT_800d1df0;

void Video_HandleStepping() {
	int i, v;

	if (GVInfo1.VideoMode != 2 || DemoTimer == 0) {
		if (DemoTimer != 0)
			return;

		for (i = 0; i < 2; i++) {
			if (Pad[i].Grind.Triggered || Pad[i].Flip.Triggered ||
			    Pad[i].SpinLeft.Triggered || Pad[i].TapSpinLeft.Triggered ||
			    Pad[i].SpinRight.Triggered || Pad[i].TapSpinRight.Triggered ||
			    Pad[i].Left.Triggered || Pad[i].Right.Triggered ||
			    Pad[i].Up.Triggered || Pad[i].Down.Triggered ||
			    Pad[i].Grab.Triggered || Pad[i].Camera.Triggered)
				Pad[i].Ollie.Triggered = 1;
		}

		return;
	}

	if (Status == 20 || Status == 22 || Status == 21)
		return;

	if (Pad[0].Flip.Triggered ||
	    (GNumberOfPlayers == 2 && Pad[1].Flip.Triggered)) {
		v = GVInfo1.PlaybackFrozen;
		Pad[0].Flip.Triggered = 0;
		Pad[1].Flip.Triggered = 0;
		v ^= 1;
		GVInfo1.PlaybackFrozen = v;
		GameFrozen = v;
		MusicStopped = v;
	}

	if (GVInfo1.PlaybackFrozen == 0)
		return;

	GameFrozen = 1;

	if (Pad[0].Grab.Pressed ||
	    (GNumberOfPlayers == 2 && Pad[1].Grab.Pressed)) {
		if (DAT_800d1df0 != 0) {
			DAT_800d1df0--;
			return;
		}

		DAT_800d1df0 = 6;
		GameFrozen = 0;
		return;
	}

	if (Pad[0].Grind.Pressed ||
	    (GNumberOfPlayers == 2 && Pad[1].Grind.Pressed)) {
		if (DAT_800d1df0 != 0) {
			DAT_800d1df0--;
			return;
		}

		DAT_800d1df0 = 1;
		GameFrozen = 0;
		return;
	}

	DAT_800d1df0 = 0;
	return;
}

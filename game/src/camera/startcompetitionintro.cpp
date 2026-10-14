/* StartCompetitionIntro__7CCamera (0x8007c814) */
/* C++ (CAMERA.CPP). Switches to the competition-intro camera mode and resets the marquee. */
#include "game/camera.h"

void Competition_ResetMarquee();

void CCamera::StartCompetitionIntro() {
	mCompIntroTimer = 0;
	mCameraMode = CAMERAMODE_COMPETITIONINTRO;
	Competition_ResetMarquee();
}

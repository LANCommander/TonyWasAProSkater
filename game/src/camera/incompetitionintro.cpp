/* InCompetitionIntro__7CCamera (0x8007cd64) */
/* C++ (CAMERA.CPP). True while the camera runs the competition intro. */
#include "game/camera.h"

int CCamera::InCompetitionIntro() {
	return mCameraMode == CAMERAMODE_COMPETITIONINTRO;
}

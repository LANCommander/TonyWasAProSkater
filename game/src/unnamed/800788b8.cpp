/* _._7CCamera (0x800788b8) flags=-fstructure-noalias */
/* C++ (CAMERA.CPP). Unlinks the camera from CameraList; the member quaternions/CamPos and CBody's
   destructor do the rest. */
#include "game/camera.h"

extern CBody *CameraList;
extern int NumCameras;

CCamera::~CCamera() {
	DeleteFrom(&CameraList);

	NumCameras--;
}

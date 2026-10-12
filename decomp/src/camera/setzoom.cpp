/* SetZoom__7CCameraUsUs (0x8007b0c4) */
/* C++ (CAMERA.CPP). Zooms to `zoom` over `time` frames (a per-frame increment), or at once if time is 0. */
#include "game/camera.h"

void CCamera::SetZoom(ushort zoom, ushort time) {
	mZoomTime = time;

	if (time)
		mZoomIncrement = (zoom - mZoom) / mZoomTime;
	else
		mZoom = zoom;
}

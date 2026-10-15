/* GameFMV_ScaleMusicVolume__Fi (0x80023fb0) */
#include "names.h"

extern int GameFMV_MusicScale;

int GameFMV_ScaleMusicVolume(int a) {
	int t = (a * GameFMV_MusicScale) / 128;

	if (t > 16383)
		t = 16383;

	return t;
}

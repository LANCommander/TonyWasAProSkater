/* GetFaceFlags__FP9SLineInfo (0x8002ee6c) */
#include "game/colij.h"

extern int Face_Air, Face_Skateable, Face_WallRide, Face_Visible, Face_Terrain;

void GetFaceFlags(SLineInfo *li) {
	if (li->pItem) {
		uint *f = li->pFace;
		uint fl = f[3];

		Face_Air = (fl >> 16) & 0x40;
		Face_Skateable = !(fl & 0x1000000);
		Face_WallRide = !(fl & 0x800000);
		Face_Visible = f[0] & 0x80;
		Face_Terrain = ((fl >> 16) & 0x1e00) >> 9;
	}
}

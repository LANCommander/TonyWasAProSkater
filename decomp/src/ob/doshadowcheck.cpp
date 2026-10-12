/* DoShadowCheck__6CBruce (0x800427d4) flags=-fstructure-noalias */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/colij.h"
#include "game/bruce.h"

extern int GNumberOfPlayers;

void M3dColij_InitLineInfo(SLineInfo *);
int M3dZone_LineToItem(SLineInfo *, int);

void CBruce::DoShadowCheck() {
	SLineInfo line_info;

	FRUVectorsFromRotMatrix();

	if (GNumberOfPlayers == 1) {
		line_info.StartCoords.vx = mPos.vx + mUp.vx * 50;
		line_info.StartCoords.vy = mPos.vy + mUp.vy * 50;
		line_info.StartCoords.vz = mPos.vz + mUp.vz * 50;
	} else {
		line_info.StartCoords.vx = mPos.vx;
		line_info.StartCoords.vy = mPos.vy;
		line_info.StartCoords.vz = mPos.vz;
	}

	line_info.EndCoords.vx = line_info.StartCoords.vx;
	line_info.EndCoords.vy = line_info.StartCoords.vy + 0xfa0000;
	line_info.EndCoords.vz = line_info.StartCoords.vz;
	M3dColij_InitLineInfo(&line_info);
	M3dZone_LineToItem(&line_info, 1);

	if (line_info.pItem && line_info.Normal.vy < -1900 && mPhysicsState != 4 && mPhysicsState != 8) {
		{
			int _x = line_info.Position.vx;
			int _y = line_info.Position.vy;
			int _z = line_info.Position.vz;

			mShadowPos.vx = _x;
			mShadowPos.vy = _y;
			mShadowPos.vz = _z;
		}

		{
			int _x = line_info.Normal.vx;
			int _z = line_info.Normal.vz;
			int _y = line_info.Normal.vy;

			mShadowNormal.vx = _x;
			mShadowNormal.vy = _y;
			mShadowNormal.vz = _z;
		}

		if (GNumberOfPlayers == 1) {
			mShadowLights = 1;
			mpShadowRGB->r = mpShadowRGB->g = mpShadowRGB->b = ((4300 - line_info.Distance) << 7) / 4000;
		}
	} else {
		mShadowLights = 0;
	}
}

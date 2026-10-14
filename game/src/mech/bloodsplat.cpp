/* BloodSplat__6CBruceii (0x80042ab0) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/colij.h"
#include "game/bruce.h"
#include "game/bit.h"

extern int Cheat_Blood;
extern int BitCount;
extern int UseCacheForBits;
extern int SparkSemiTrans;

void Video_BloodSplat(struct CBruce *, int, int);
void Bit_SetSparkTrajectory(const CSVector &);
void Bit_SetSparkTrajectoryCone(const CSVector &);
void Bit_SetSparkSize(uint);
void Bit_SetSparkRGB(uchar, uchar, uchar);
void Bit_SetSparkFadeRGB(uchar, uchar, uchar);
int Mem_GetFree(char);
void M3dColij_InitLineInfo(SLineInfo *);
int M3dZone_LineToItem(SLineInfo *, int);

void CBruce::BloodSplat(int BodyPart, int MaxBloodBits) {
	if (Cheat_Blood) {
		Video_BloodSplat(this, BodyPart, MaxBloodBits);

		if (BitCount < 200) {
			CSVector v;
			CVector Pos;
			CSVector Cone;
			int Bits;
			int FloorY;
			SLineInfo BloodLine;

			v.vx = -1024;
			v.vy = 0;
			v.vz = 0;
			Bit_SetSparkTrajectory(v);

			if (BodyPart == 0) {
				Cone.vx = 1200;
				Cone.vy = 800;
				Cone.vz = 0;
			} else if (BodyPart == 1) {
				Cone.vx = 1600;
				Cone.vy = 1600;
				Cone.vz = 0;
			} else {
				Cone.vx = 1200;
				Cone.vy = 800;
				Cone.vz = 0;
			}

			Bit_SetSparkTrajectoryCone(Cone);
			Bit_SetSparkRGB(72, 0, 0);
			Bit_SetSparkFadeRGB(0, 0, 0);
			GetHookPosition(Pos, BodyPart);

			SparkSemiTrans = 0;
			UseCacheForBits = 0;
			Bits = 200 - BitCount;

			if (MaxBloodBits < Bits)
				Bits = MaxBloodBits;

			BloodLine.StartCoords.vx = Pos.vx;
			BloodLine.StartCoords.vy = Pos.vy - 163840;
			BloodLine.StartCoords.vz = Pos.vz;
			BloodLine.EndCoords.vx = Pos.vx;
			BloodLine.EndCoords.vy = Pos.vy + 3276800;
			BloodLine.EndCoords.vz = Pos.vz;
			M3dColij_InitLineInfo(&BloodLine);
			M3dZone_LineToItem(&BloodLine, 1);

			if (BloodLine.pItem) {
				FloorY = BloodLine.Position.vy - 16384;
			} else {
				FloorY = Pos.vy - 16384;
			}

			Pos.vy -= 122880;

			{
				int i;

				for (i = 0; i < Bits; i++) {
					CSpark *pSpark;

					if (Mem_GetFree(1) < 10000)
						return;

					Bit_SetSparkSize((i & 1) + 1);

					pSpark = new CSpark(Pos, (i >> 2) + 14, 10240 + i * 256, (i & 7) + 50);

					pSpark->mVel += mVel;
					pSpark->mFloorY = FloorY;
				}
			}

			UseCacheForBits = 1;
			SparkSemiTrans = 1;
		}
	}
}

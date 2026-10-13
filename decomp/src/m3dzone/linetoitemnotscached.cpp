/* M3dZone_LineToItemNotSCached__FP9SLineInfoi (0x800827d4) */
/* C++ (M3DZONE.CPP). Demo twin (opcode-identical but for one lui); locals from its MAIN.SYM.
   Each zone is a box split into up to 20x20 cells; the line is clipped to the box and walked
   cell by cell with a DDA.
   Levers: the cell subtractions (x0 - xL etc.) are written inside the divisions; egcs's PRE
   hoists them to just after the last clip that changes their operands (the demo line table puts
   them on the z1 > zH test), which is what frees a register for zL. The single-cell case uses X/Z
   (not X0/Z0), raising X/Z's priority to s0/s1. The walk's bounds checks are part of the while
   condition (one source line in the demo). */
#include "game/colij.h"
#include "game/m3dasm.h"

extern SZone Zones[2];
extern Item *EnvironmentalObjectList;
extern int CameraCollisionCheck;
extern int BaddyCollisionCheck;
extern int BulletCollisionCheck;
extern int TriggerCollisionCheck;
extern int LineOfSightCheck;
extern int M3dColij_ZeroMask;
extern int M3dColij_OneMask;

void M3dColij_LineToItem(Item *, SLineInfo *);
int M3dColij_GetLineInfo(SLineInfo *);

int M3dZone_LineToItemNotSCached(SLineInfo *pInfo, int CheckEnviroObs) {
	int x0, x1, z0, z1;
	int xL, xH, zL, zH;
	int X0, X1, Z0, Z1;
	int r0, s0;
	int X, Z;
	int dX, dZ;
	int w, h, l, d;
	int qh, qw;

	M3dColij_OneMask = 0;
	M3dColij_ZeroMask = -1;

	if (LineOfSightCheck)
		M3dColij_OneMask = 0x400000;

	if (BulletCollisionCheck)
		M3dColij_OneMask ^= 0x400000;

	if (!BaddyCollisionCheck)
		M3dColij_OneMask ^= 0x200000;

	if (CameraCollisionCheck)
		M3dColij_ZeroMask = 0xffefffff;

	if (TriggerCollisionCheck)
		M3dColij_ZeroMask ^= 0x20000;

	if (CheckEnviroObs)
		M3dColij_LineToItem(EnvironmentalObjectList, pInfo);

	{
		int EnvIndex;

		for (EnvIndex = 0; EnvIndex < 2; EnvIndex++) {
			if (!Zones[EnvIndex].Flags)
				continue;

			x0 = pInfo->StartCoords.vx;
			x1 = pInfo->EndCoords.vx;
			z0 = pInfo->StartCoords.vz;
			z1 = pInfo->EndCoords.vz;
			xL = Zones[EnvIndex].xMin;
			xH = Zones[EnvIndex].xMax;
			zL = Zones[EnvIndex].zMin;
			zH = Zones[EnvIndex].zMax;
			l = Zones[EnvIndex].ZoneWidth;

			if ((x0 < xL && x1 < xL) || (x0 > xH && x1 > xH) || (z0 < zL && z1 < zL) ||
			    (z0 > zH && z1 > zH))
				continue;

			if (x0 == x1 && z0 == z1) {
				X = (x0 - xL) / l;
				Z = (z0 - zL) / l;

				if (X == Zones[EnvIndex].Width)
					X--;

				if (Z == Zones[EnvIndex].Height)
					Z--;

				M3dColij_LineToItemZoned((Item **)Zones[EnvIndex].Ptr[X][Z], pInfo);
				continue;
			}

			if (x0 < xL) {
				z0 = z0 < z1 ? z0 + M3dMaths_MulDiv64(xL - x0, z1 - z0, x1 - x0)
				             : z1 + M3dMaths_MulDiv64(x1 - xL, z0 - z1, x1 - x0);
				x0 = xL;
			}

			if (x1 < xL) {
				z1 = z1 < z0 ? z1 + M3dMaths_MulDiv64(xL - x1, z0 - z1, x0 - x1)
				             : z0 + M3dMaths_MulDiv64(x0 - xL, z1 - z0, x0 - x1);
				x1 = xL;
			}

			if (x0 > xH) {
				z0 = z1 < z0 ? z1 + M3dMaths_MulDiv64(xH - x1, z0 - z1, x0 - x1)
				             : z0 + M3dMaths_MulDiv64(x0 - xH, z1 - z0, x0 - x1);
				x0 = xH;
			}

			if (x1 > xH) {
				z1 = z0 < z1 ? z0 + M3dMaths_MulDiv64(xH - x0, z1 - z0, x1 - x0)
				             : z1 + M3dMaths_MulDiv64(x1 - xH, z0 - z1, x1 - x0);
				x1 = xH;
			}

			if (z0 < zL) {
				x0 = x0 < x1 ? x0 + M3dMaths_MulDiv64(zL - z0, x1 - x0, z1 - z0)
				             : x1 + M3dMaths_MulDiv64(z1 - zL, x0 - x1, z1 - z0);
				z0 = zL;
			}

			if (z1 < zL) {
				x1 = x1 < x0 ? x1 + M3dMaths_MulDiv64(zL - z1, x0 - x1, z0 - z1)
				             : x0 + M3dMaths_MulDiv64(z0 - zL, x1 - x0, z0 - z1);
				z1 = zL;
			}

			if (z0 > zH) {
				x0 = x1 < x0 ? x1 + M3dMaths_MulDiv64(zH - z1, x0 - x1, z0 - z1)
				             : x0 + M3dMaths_MulDiv64(z0 - zH, x1 - x0, z0 - z1);
				z0 = zH;
			}

			if (z1 > zH) {
				x1 = x0 < x1 ? x0 + M3dMaths_MulDiv64(zH - z0, x1 - x0, z1 - z0)
				             : x1 + M3dMaths_MulDiv64(z1 - zH, x0 - x1, z1 - z0);
				z1 = zH;
			}

			w = x1 - x0;

			if (w < 0) {
				dX = -1;
				w = -w;
			} else
				dX = 1;

			h = z1 - z0;

			if (h < 0) {
				dZ = -1;
				h = -h;
			} else
				dZ = 1;

			X0 = (x0 - xL) / l;
			X1 = (x1 - xL) / l;
			Z0 = (z0 - zL) / l;
			Z1 = (z1 - zL) / l;
			r0 = (x0 - xL) % l;
			s0 = (z0 - zL) % l;

			if (X0 == Zones[EnvIndex].Width) {
				X0--;
				r0 += l;
			}

			if (Z0 == Zones[EnvIndex].Height) {
				Z0--;
				s0 += l;
			}

			if (X1 == Zones[EnvIndex].Width)
				X1--;

			if (Z1 == Zones[EnvIndex].Height)
				Z1--;

			qw = M3dMaths_MulDiv64(w, s0, l);
			qh = M3dMaths_MulDiv64(h, r0, l);
			X = X0;
			Z = Z0;
			d = (x0 < x1 ? qh - h : -qh) + (z0 < z1 ? w - qw : qw);

			while ((X != X1 || Z != Z1) && X >= 0 && X < Zones[EnvIndex].Width && Z >= 0 &&
			       Z < Zones[EnvIndex].Height) {
				M3dColij_LineToItemZoned((Item **)Zones[EnvIndex].Ptr[X][Z], pInfo);

				if (d >= 0) {
					d -= h;
					X += dX;
				} else {
					d += w;
					Z += dZ;
				}
			}

			if (X >= 0 && X < Zones[EnvIndex].Width && Z >= 0 && Z < Zones[EnvIndex].Height)
				M3dColij_LineToItemZoned((Item **)Zones[EnvIndex].Ptr[X][Z], pInfo);
		}
	}

	return M3dColij_GetLineInfo(pInfo);
}

/* M3dZone_LineToItemNotSCached__FP9SLineInfoi (0x800827d4) */
#include "names.h"

typedef struct {
	int x0, y0, z0, x1, y1, z1;
} SLine;

typedef struct {
	int active;
	int minx, minz, maxx, maxz;
	int cell;
	int pad;
	short nx, nz;
	int cells[20][20];
} SGrid;

extern SGrid Zones[2];
extern int M3dColij_OneMask, M3dColij_ZeroMask, LineOfSightCheck, BulletCollisionCheck,
    BaddyCollisionCheck, CameraCollisionCheck, TriggerCollisionCheck;
extern int EnvironmentalObjectList;

int FUN_8008b62c(int, int, int);
void M3dColij_LineToItemZoned(int, SLine *);
void M3dColij_LineToItem(int, SLine *);
void M3dColij_GetLineInfo(SLine *);

void M3dZone_LineToItemNotSCached(SLine *line, int items) {
	int z0, x1, x0, z1;
	int minx, minz, maxx, maxz, cell;
	int cx, cz, ex, ez, rx, rz, dx, dz, sx, sz, err, a, b;
	int ox0, oz0, ox1, oz1, x, z, t, e;
	int i;

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

	if (items)
		M3dColij_LineToItem(EnvironmentalObjectList, line);

	for (i = 0; i < 2; i++) {
		if (!Zones[i].active)
			continue;

		x0 = line->x0;
		x1 = line->x1;
		z0 = line->z0;
		z1 = line->z1;
		minx = Zones[i].minx;
		maxx = Zones[i].maxx;
		minz = Zones[i].minz;
		maxz = Zones[i].maxz;
		cell = Zones[i].cell;

		if (x0 < minx && x1 < minx)
			continue;

		if (maxx < x0 && maxx < x1)
			continue;

		if (z0 < minz && z1 < minz)
			continue;

		if (maxz < z0 && maxz < z1)
			continue;

		if (x0 == x1 && z0 == z1) {
			cx = (x0 - minx) / cell;
			cz = (z0 - minz) / cell;

			if (cx == Zones[i].nx)
				cx--;

			if (cz == Zones[i].nz)
				cz--;

			M3dColij_LineToItemZoned(Zones[i].cells[cx][cz], line);
			continue;
		}

		if (x0 < minx) {
			z0 = (z0 < z1) ? z0 + FUN_8008b62c(minx - x0, z1 - z0, x1 - x0)
			               : z1 + FUN_8008b62c(x1 - minx, z0 - z1, x1 - x0);
			x0 = minx;
		}

		if (x1 < minx) {
			z1 = (z1 < z0) ? z1 + FUN_8008b62c(minx - x1, z0 - z1, x0 - x1)
			               : z0 + FUN_8008b62c(x0 - minx, z1 - z0, x0 - x1);
			x1 = minx;
		}

		if (maxx < x0) {
			z0 = (z1 < z0) ? z1 + FUN_8008b62c(maxx - x1, z0 - z1, x0 - x1)
			               : z0 + FUN_8008b62c(x0 - maxx, z1 - z0, x0 - x1);
			x0 = maxx;
		}

		if (maxx < x1) {
			z1 = (z0 < z1) ? z0 + FUN_8008b62c(maxx - x0, z1 - z0, x1 - x0)
			               : z1 + FUN_8008b62c(x1 - maxx, z0 - z1, x1 - x0);
			x1 = maxx;
		}

		if (z0 < minz) {
			x0 = (x0 < x1) ? x0 + FUN_8008b62c(minz - z0, x1 - x0, z1 - z0)
			               : x1 + FUN_8008b62c(z1 - minz, x0 - x1, z1 - z0);
			z0 = minz;
		}

		if (z1 < minz) {
			x1 = (x1 < x0) ? x1 + FUN_8008b62c(minz - z1, x0 - x1, z0 - z1)
			               : x0 + FUN_8008b62c(z0 - minz, x1 - x0, z0 - z1);
			z1 = minz;
		}

		if (maxz < z0) {
			x0 = (x1 < x0) ? x1 + FUN_8008b62c(maxz - z1, x0 - x1, z0 - z1)
			               : x0 + FUN_8008b62c(z0 - maxz, x1 - x0, z0 - z1);
			z0 = maxz;
		}

		if (maxz < z1) {
			x1 = (x0 < x1) ? x0 + FUN_8008b62c(maxz - z0, x1 - x0, z1 - z0)
			               : x1 + FUN_8008b62c(z1 - maxz, x0 - x1, z1 - z0);
			z1 = maxz;
		}

		dx = x1 - x0;
		ox1 = x1 - minx;
		oz1 = z1 - minz;

		if (dx < 0) {
			sx = -1;
			dx = -dx;
		} else
			sx = 1;

		dz = z1 - z0;

		if (dz < 0) {
			sz = -1;
			dz = -dz;
		} else
			sz = 1;

		ex = ox1 / cell;
		cx = (x0 - minx) / cell;
		rx = (x0 - minx) % cell;
		cz = (z0 - minz) / cell;
		rz = (z0 - minz) % cell;
		ez = oz1 / cell;

		if (cx == Zones[i].nx) {
			cx--;
			rx += cell;
		}

		if (cz == Zones[i].nz) {
			cz--;
			rz += cell;
		}

		if (ex == Zones[i].nx)
			ex--;

		if (ez == Zones[i].nz)
			ez--;

		a = FUN_8008b62c(dx, rz, cell);
		b = FUN_8008b62c(dz, rx, cell);

		if (x0 < x1)
			e = b - dz;
		else
			e = -b;

		err = (z0 < z1) ? e + (dx - a) : e + a;
		x = cx;
		z = cz;

		while ((x != ex || z != ez) && x >= 0 && x < Zones[i].nx && z >= 0 && z < Zones[i].nz) {
			M3dColij_LineToItemZoned(Zones[i].cells[x][z], line);

			if (err >= 0) {
				err -= dz;
				x += sx;
			} else {
				err += dx;
				z += sz;
			}
		}

		if (x >= 0 && x < Zones[i].nx && z >= 0 && z < Zones[i].nz)
			M3dColij_LineToItemZoned(Zones[i].cells[x][z], line);
	}

	M3dColij_GetLineInfo(line);
}

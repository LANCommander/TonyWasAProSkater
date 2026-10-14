/* Utils_CalculateSpatialAttenuation__FRC7CVectorii (0x800489e4) */
/* C++ (UTILS.CPP). Demo twin; locals from its MAIN.SYM. Volume for a sound at pos heard from the
   current camera: full inside near, silent past far, linear between; then panned left/right by
   the source's heading in camera space (quieter and flagged when it is behind). The sine table is
   a sized extern so its address is one `la` macro: reorg then fills the branch delay slot with the
   index shift, as retail does. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/vector.h"
#include "game/trig.h"

struct SMikeCam {
	int pad;
	CVector Pos; /* 0x04 */
	char pad10[0x74 - 0x10];
	MATRIX Rot; /* 0x74 */
	char pad94[164 - 0x94];
};

extern SMikeCam MikeCamera[];
extern SSinCos rcossin_tbl;
extern int Camera_CurrentMikeCamera;

int Utils_CrapDist(const CVector &, const CVector &);

int Utils_CalculateSpatialAttenuation(const CVector &pos, int near, int far) {
	int vol;
	CVector cam_pos(MikeCamera[Camera_CurrentMikeCamera].Pos.vx << 12,
	                MikeCamera[Camera_CurrentMikeCamera].Pos.vy << 12,
	                MikeCamera[Camera_CurrentMikeCamera].Pos.vz << 12);
	int d = Utils_CrapDist(pos, cam_pos);
	CVector centre;
	short heading;
	uint behind;
	uint left;
	uint right;

	if (near >= d)
		return 0x0fff0fff;

	if (d >= far)
		return 0;

	vol = (((far - d) << 12) - (far - d)) / far;
	geom_ld_mat(pc_geom.rot, &MikeCamera[Camera_CurrentMikeCamera].Rot);

	centre.vx = (pos.vx >> 12) - MikeCamera[Camera_CurrentMikeCamera].Pos.vx;
	centre.vy = (pos.vy >> 12) - MikeCamera[Camera_CurrentMikeCamera].Pos.vy;
	centre.vz = (pos.vz >> 12) - MikeCamera[Camera_CurrentMikeCamera].Pos.vz;
	{
		/* into camera space: the saturated 16-bit results, as words */
		int16_t v[3];
		int32_t w[3];
		GeomAcc r;

		geom_ld_lv(v, &centre);
		r = geom_mvmva(pc_geom.rot, 0, v[0], v[1], v[2], 12, 0, 0);
		geom_load_vector(w, r.ir[0], r.ir[1], r.ir[2]);
		geom_st_lv(&centre, w);
	}

	if (!(centre.vx | centre.vz))
		return vol | (vol << 16);

	heading = (1024 - ratan2(centre.vz, centre.vx)) & 4095;
	behind = 0;

	if (heading > 1024 && heading < 3072) {
		behind = 0x80000000;
		vol -= vol >> 4;
	}

	if (heading < 2048) {
		left = vol - ((vol * (&rcossin_tbl)[heading].s) >> 12);
		right = vol;
	} else {
		left = vol;
		right = left + ((vol * (&rcossin_tbl)[heading].s) >> 12);
	}

	return left | (right << 16) | behind;
}

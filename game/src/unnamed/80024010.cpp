/* GameFMV_SetCDMixForPos__FRC7CVector (0x80024010) */
/* C++ (GAMEFMV). Sets the CD-audio mixer for a sound at pos: the spatial attenuation's left/right
   (12 bits each) are scaled by the current volume and added to a 3/4-volume base. */
#include "game/gamefmv.h"
#include "game/psyq.h"
#include "game/vector.h"
#include "pc_stream.h"

struct CdlATV {
	unsigned char val0, val1, val2, val3;
};

extern int XALEVEL;

unsigned int Utils_CalculateSpatialAttenuation(const CVector &, int, int);

void GameFMV_SetCDMixForPos(const CVector &pos) {
	CdlATV atv;
	unsigned int att = Utils_CalculateSpatialAttenuation(pos, 1024, 16384);
	int vol = GameFMV_ScaleMusicVolume(XALEVEL);
	int l = ((att & 0xfff) * vol) >> 18;
	int r = (((att >> 16) & 0xfff) * vol) >> 18;
	int base;

	if (l > 255)
		l = 255;

	if (r > 255)
		r = 255;

	base = (vol * 192) >> 14;
	l = base + l / 4;
	r = base + r / 4;
	atv.val0 = l;
	atv.val1 = r;
	atv.val3 = 0;
	atv.val2 = 0;

	PC_StreamMix(&atv.val0);
}

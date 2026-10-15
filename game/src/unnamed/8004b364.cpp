/* SFX_PlayLR__FUissi (0x8004b364) */
/* C++ (PAL). Stereo variant of FUN_8004b250: plays sound effect id with separate left/right
   volumes (each clamped to 16384), scaled by the global percentage SFXLEVEL. */
#include "game/sfx.h"

extern int DAT_800b8a9c[];
extern int SFXLEVEL;

extern int SFX_v;

int SFX_PlayLR(uint id, short l, short r, int pan) {
	int p, note;

	if (l > 16384)
		l = 16384;

	if (r > 16384)
		r = 16384;

	if ((int)id < 0 || id == 0xffff)
		return 0;

	p = DAT_800b8a9c[id * 2 + 1];
	note = 60;

	if (p & 0xffff0000) {
		note = p >> 16;
		p &= 0xffff;
	}

	SFX_v = p;
	p = SFXLEVEL * p / 100;

	return playSFX(DAT_800b8a9c[id * 2], note, (l * p * 4) >> 16, (r * p * 4) >> 16, pan);
}

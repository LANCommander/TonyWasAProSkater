/* SFX_SetReverbType__FUc (0x8004ba7c) */
#include "pc_sound.h"
typedef struct {
	short left, right;
} VOL;

typedef struct {
	int mask;
	int mode;
	VOL depth;
	int delay;
	int feedback;
} ATTR;

extern int FUN_80093914(ATTR *);
extern int FUN_80094ca4(int);
extern int FUN_80094c74(int, int);

int SFX_ReverbDepthSetTimer;

void SFX_SetReverbType(unsigned char t) {
	ATTR a;

	a.mask = 1;
	a.mode = t;

	/* PC: the native reverb (pc_sound.h); its depth is set when the timer runs out */
	if (!PC_SoundReverbMode(t))
		return;

	SFX_ReverbDepthSetTimer = t ? 0xf0 : 0;
	PC_SoundReverbChannels(t != 0, 0xffffff);
	return;
	if (FUN_80093914(&a) == -1)
		return;

	if (t == 0) {
		SFX_ReverbDepthSetTimer = 0;
		FUN_80094c74(0, 0xffffff);
	} else if (FUN_80094ca4(1)) {
		SFX_ReverbDepthSetTimer = 0xf0;
		FUN_80094c74(1, 0xffffff);
	} else
		FUN_80094c74(0, 0xffffff);
}

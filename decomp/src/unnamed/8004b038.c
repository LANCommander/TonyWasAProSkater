/* SFX_InitAtStart__Fv (0x8004b038) */
#include "names.h"
#ifdef PC_PORT
#include "pc_sound.h"
#endif

struct S {
	int a;
	short b, c;
	int p1, p2;
	short h, i;
	int j, k;
	int l, m;
};

int pSFXVABHeader;
int pLevelSpecificSFXVABHeader;
int DAT_800d1a94;
int DAT_800d1a98;

void FUN_80094d74(void);
void FUN_80094444(struct S *s);

void SFX_InitAtStart(void) {
	struct S s;

#ifdef PC_PORT
	PC_SoundReverbMode(0);
#else
	FUN_80094d74();
#endif

	s.a = 963;
	s.b = 16383;
	s.c = 16383;
	s.h = 32512;
	s.i = 32512;
	s.j = 0;
	s.k = 1;
#ifdef PC_PORT
	/* the music's volume was a full-scale one on the PS1 (the voices' are halved) */
	PC_SoundLevels(s.b, s.c, s.h / 2, s.i / 2);
#else
	FUN_80094444(&s);
#endif

	pSFXVABHeader = 0;
	pLevelSpecificSFXVABHeader = 0;
	DAT_800d1a94 = -1;
	DAT_800d1a98 = -1;
}

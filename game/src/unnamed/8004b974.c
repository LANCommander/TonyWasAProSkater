/* SFX_Unpause__Fv (0x8004b974) */
#include "names.h"
#include "pc_stream.h"
#include "pc_sound.h"

struct S {
	int a, b, c, d, e;
	short f;
	char pad[42];
};

int SFXPaused;

extern unsigned short pitchPriorToPause[];

void SpuNSetVoiceAttr(int a, struct S *s);
void FUN_8008f3a0(int a, int b);

void SFX_Unpause(void) {
	struct S s;
	int i;
	unsigned short *q;

	if (SFXPaused) {
		i = 0;
		q = pitchPriorToPause;
		SFXPaused = 0;
		s.b = 16;

		for (; i < 24; i++) {
			s.f = *q;
			PC_SoundChannelPitch(i, s.f);

			q++;
		}

		PC_StreamMute(0);
	}
}

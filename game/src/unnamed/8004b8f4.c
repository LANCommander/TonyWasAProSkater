/* SFX_Pause__Fv (0x8004b8f4) */
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

void FUN_80094424(int a, unsigned short *b);
void SpuNSetVoiceAttr(int a, struct S *s);
void FUN_8008f3a0(int a, int b);

void SFX_Pause(void) {
	struct S s;
	int i;
	unsigned short *q;

	if (!SFXPaused) {
		i = 0;
		q = pitchPriorToPause;
		SFXPaused = 1;
		s.b = 16;
		s.f = 0;

		for (; i < 24; i++) {
			*q = (unsigned short)PC_SoundChannelGetPitch(i);
			PC_SoundChannelPitch(i, 0);

			q++;
		}

		PC_StreamMute(1);
	}
}

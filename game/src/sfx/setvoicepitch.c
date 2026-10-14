/* SFX_SetVoicePitch__FUiUs (0x8004b5d0) */
#include "pc_sound.h"
typedef struct {
	short left, right;
} VOL;

typedef struct {
	unsigned long voice;
	unsigned long mask;
	VOL volume;
	VOL volmode;
	VOL volumex;
	unsigned short pitch;
	unsigned short note;
	unsigned short sample_note;
	short envx;
	unsigned long addr;
	unsigned long loop_addr;
	long a_mode;
	long s_mode;
	long r_mode;
	unsigned short ar, dr, sr, rr, sl, adsr1, adsr2;
} VATTR;

int SFXPaused;

extern void SpuNSetVoiceAttr(int, VATTR *);

void SFX_SetVoicePitch(unsigned m, unsigned short p) {
	VATTR a;
	int i;

	if (SFXPaused)
		return;

	if (!m)
		return;

	for (i = 0; i < 24; i++) {
		if (m & (1 << i)) {
			PC_SoundChannelPitch(i, p);
		}
	}
}

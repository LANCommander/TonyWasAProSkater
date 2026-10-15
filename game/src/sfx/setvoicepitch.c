/* SFX_SetVoicePitch__FUiUs (0x8004b5d0) */
#include "pc_sound.h"
typedef struct {
	short left, right;
} VOL;

typedef struct {
	unsigned int voice;
	unsigned int mask;
	VOL volume;
	VOL volmode;
	VOL volumex;
	unsigned short pitch;
	unsigned short note;
	unsigned short sample_note;
	short envx;
	unsigned int addr;
	unsigned int loop_addr;
	int a_mode;
	int s_mode;
	int r_mode;
	unsigned short ar, dr, sr, rr, sl, adsr1, adsr2;
} VATTR;

extern int SFXPaused;

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

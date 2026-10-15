/* SFX_GetVoicePitch__FUi (0x8004b590) */
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

extern void FUN_80094f70(VATTR *);

int SFX_GetVoicePitch(unsigned v) {
	VATTR a;

	if (SFXPaused)
		return 1000;

	if (v) {
		int i = 0;

		while (!(v >> i & 1))
			i++;

		return PC_SoundChannelGetPitch(i);
	}
}

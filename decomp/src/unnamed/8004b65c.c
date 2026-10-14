/* SFX_ModifyPos__FUiRC7CVectori (0x8004b65c) */
#include "names.h"
#ifdef PC_PORT
#include "pc_sound.h"
#endif

typedef struct {
	int id;
	int cmd;
	short l, r;
	char pad[8];
	short v;
	char pad2[42];
} S;

extern int SFXLEVEL;

unsigned short SFXFalloffArray[48];

void SpuNSetVoiceAttr(int, S *);
unsigned Utils_CalculateSpatialAttenuation(void *, int, int);

void SFX_ModifyPos(unsigned a, void *pos, int b) {
	S s;
	unsigned t, l, r;
	int i, y, v, d;
	unsigned short x;
	unsigned short *tab;

	if (a) {
		t = Utils_CalculateSpatialAttenuation(pos, 256, 12000);
		l = ((t & 0xfff) * SFXLEVEL) >> 14;
		r = (((t >> 16) & 0xfff) * SFXLEVEL) >> 14;

		if ((int)t < 0)
			l = -l;

		i = 0;
		tab = SFXFalloffArray;

		for (; i < 24; i++) {
			if (a & (1 << i)) {
				s.id = i;
				s.cmd = 3;

				if ((int)l < -16383)
					l = -16383;

				if ((int)l >= 16384)
					l = 16383;

				if ((int)r < -16383)
					r = -16383;

				if ((int)r >= 16384)
					r = 16383;

				s.l = l;
				s.r = r;

				if (b) {
					x = *tab;
					s.cmd = 19;
					y = x - (x * b / 8192);

					if (x < y) {
						d = y - x;
						v = x + (x >> 1);

						if (v < d)
							y = v;
					}

					s.v = y;
				}

#ifdef PC_PORT
				PC_SoundChannelVolume(i, s.l, s.r);

				if (s.cmd & 16)
					PC_SoundChannelPitch(i, (unsigned short)s.v);
#else
				SpuNSetVoiceAttr(i, &s);
#endif
			}

			tab += 1;
		}
	}
}

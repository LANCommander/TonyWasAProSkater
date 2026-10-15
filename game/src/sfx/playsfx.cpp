/* playSFX__FUiUcssi (0x8004bb74) */
#include "pc_sound.h"
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>

	struct SpuVolume {
		short left;
		short right;
	};

	struct SpuVoiceAttr {
		unsigned int voice;
		unsigned int mask;
		SpuVolume volume;
		SpuVolume volmode;
		SpuVolume volumex;
		unsigned short pitch;
		unsigned short note;
		unsigned short sample_note;
		short envx;
		unsigned int addr;
		unsigned int loop_addr;
		int a_mode;
		int s_mode;
		int r_mode;
		unsigned short ar;
		unsigned short dr;
		unsigned short sr;
		unsigned short rr;
		unsigned short sl;
		unsigned short adsr1;
		unsigned short adsr2;
	};

	struct VagAtr {
		unsigned char prior;
		unsigned char mode;
		unsigned char vol;
		unsigned char pan;
		unsigned char center;
		unsigned char shift;
		unsigned char min;
		unsigned char max;
		unsigned char vibW;
		unsigned char vibT;
		unsigned char porW;
		unsigned char porT;
		unsigned char pbmin;
		unsigned char pbmax;
		unsigned char reserved1;
		unsigned char reserved2;
		unsigned short adsr1;
		unsigned short adsr2;
		short prog;
		short vag;
		short reserved[4];
	};
}

#include "game/psyq.h"
#include "game/types.h"

int Redbook_Rnd(int);
int allocateVoice(void);

extern char *pSFXVABHeader;
extern char *pLevelSpecificSFXVABHeader;
extern unsigned nko_flag, keyon_queue, keyoff_queue, voiceon_flag, loop_flag, voicefree_flag;

extern ushort SFXFalloffArray[];

int playSFX(uint sfx, uchar pitch, short vl, short vr, int pitch_offset) {
	int looping;
	ushort prog;
	ushort tones;
	ushort voices;
	int voice_bf;

	looping = sfx >> 31;
	prog = (sfx >> 16) & 127;
	tones = sfx & 0xffff;

	if (pitch_offset) {
		Redbook_Rnd(pitch_offset * 2);
	}

	voices = 1;

	if (tones > 1) {
		voices = 0;

		{
			int t;

			for (t = 0; t < 16; t++)
				if ((tones >> t) & 1)
					voices++;
		}
	}

	voice_bf = 0;

	{
		int i;

		for (i = 0; i < voices; i++) {
			int free = allocateVoice();

			if (free == -1) {
				voice_bf = 0;
				break;
			}

			voice_bf |= 1 << free;
		}
	}

	if (voice_bf > 0) {
		{
			uint lzc;
			char *p_vab_hdr;
			int tone_start;
			SpuVoiceAttr attr;

			if (!(sfx & 0x40000000))
				p_vab_hdr = pSFXVABHeader;
			else
				p_vab_hdr = pLevelSpecificSFXVABHeader;

			lzc = geom_lzc(voice_bf); /* leading zeros */

			lzc = 32 - lzc;
			tone_start = 0;
			attr.mask = 0x60093;

			{
				int v;

				for (v = 0; v < lzc; v++) {
					if (!tones)
						break;

					if ((voice_bf >> v) & 1) {
						int t;

						for (t = tone_start; t < 16; t++) {
							if ((tones >> t) & 1) {
								VagAtr *p_vagatr;
								ushort *p_offset;

								tone_start = t + 1;
								p_vagatr = &((VagAtr *)(p_vab_hdr + 2080 + prog * 512))[t];

								if (vl < -16383)
									vl = -16383;

								if (vl > 16383)
									vl = 16383;

								if (vr < -16383)
									vr = -16383;

								if (vr > 16383)
									vr = 16383;

								attr.voice = 1 << v;
								attr.volume.left = vl;
								attr.volume.right = vr;
								attr.pitch =
								    PC_SoundNotePitch(pitch, 0, p_vagatr->center, p_vagatr->shift);
								SFXFalloffArray[v] = attr.pitch;

								{
									uint addr = *(uint *)(p_vab_hdr + 12);
									int n = p_vagatr->vag;

									p_offset = (ushort *)(p_vab_hdr + 2080 +
									                      *(ushort *)(p_vab_hdr + 18) * 512);

									while (n--)
										addr += *p_offset++ << 3;

									attr.addr = addr;
								}

								attr.adsr1 = p_vagatr->adsr1;
								attr.adsr2 = p_vagatr->adsr2;
								PC_SoundChannelSet(v, attr.addr, attr.pitch, attr.volume.left,
								                   attr.volume.right, attr.adsr1, attr.adsr2);

								if (looping)
									loop_flag |= 1 << v;

								tones &= ~(1 << t);
								voicefree_flag &= ~(1 << v);
								nko_flag |= 1 << v;
								voiceon_flag |= 1 << v;
								keyon_queue |= 1 << v;
								keyoff_queue &= ~(1 << v);
								break;
							}
						}
					}
				}
			}
		}
	}

	return voice_bf;
}

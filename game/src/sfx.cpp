/* SFX.CPP: sound effects - loading the VAB banks, playing, positioning, pitching, pausing and
   stopping their voices, and the reverb (declarations in game/sfx.h). */

#include "game/sfx.h"
#include "pc_sound.h"
#include "game/vector.h"
#include "pc_stream.h"
#include "game/psyq.h"
#include "game/types.h"
#include "game/fileio.h"
#include "game/mem.h"
#include "game/globals.h"
#include "game/utils.h"
#include "game/video.h"

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>

	/* LIBSPU.H */
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

	/* LIBSPU.H SpuCommonAttr (SFX_InitAtStart) */
	struct SpuCommonAttr {
		int mask;
		SpuVolume mvol;
		SpuVolume mvolmode;
		SpuVolume mvolx;
		SpuVolume cd_volume;
		int cd_reverb;
		int cd_mix;
		SpuVolume ext_volume;
		int ext_reverb;
		int ext_mix;
	};

	/* LIBSPU.H SpuReverbAttr */
	struct SpuReverbAttr {
		int mask;
		int mode;
		SpuVolume depth;
		int delay;
		int feedback;
	};

	/* LIBSND.H: one tone of a VAB program */
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

/* a VAB file's header (LIBSND.H VabHdr: fsize, ps): its size word holds the bank's SPU address
   once loaded */
struct SVabHead {
	char pad0[12];
	int Size; /* 0x0c */
	char pad10[2];
	unsigned short NumProgs; /* 0x12 */
};

extern char *pSFXVABHeader;
extern char *pLevelSpecificSFXVABHeader;
extern int DAT_800d1a94;
extern int DAT_800d1a98;
int FUN_8004b188(char *name, char **out);
extern int DAT_800b8a9c[];
extern int SFXPaused;
extern unsigned short SFXFalloffArray[24];
extern unsigned short pitchPriorToPause[];
extern unsigned nko_flag, loop_flag, voiceon_flag, exp_flag, voicefree_flag;
extern unsigned keyon_queue, keyoff_queue, reverb_queue;
extern unsigned char reverb_changed;
int FUN_80093914(SpuReverbAttr *);
int FUN_80094ca4(int);
void FUN_80094c74(int, int);
int allocateVoice(void);

/* SFX_InitAtStart__Fv (0x8004b038) */
void SFX_InitAtStart(void) {
	SpuCommonAttr s;

	PC_SoundReverbMode(0);

	s.mask = 963;
	s.mvol.left = 16383;
	s.mvol.right = 16383;
	s.cd_volume.left = 32512;
	s.cd_volume.right = 32512;
	s.cd_reverb = 0;
	s.cd_mix = 1;
	/* the music's volume was a full-scale one on the PS1 (the voices' are halved) */
	PC_SoundLevels(s.mvol.left, s.mvol.right, s.cd_volume.left / 2, s.cd_volume.right / 2);

	pSFXVABHeader = 0;
	pLevelSpecificSFXVABHeader = 0;
	DAT_800d1a94 = -1;
	DAT_800d1a98 = -1;
}

/* SFX_Init__FPc (0x8004b09c) */
void SFX_Init(char *a) {
	PC_SoundBanksReset();
	SFX_SetReverbType(0);

	DAT_800d1a94 = FUN_8004b188(a, &pSFXVABHeader);
}

/* FUN_8004b0e4 (0x8004b0e4) */
void FUN_8004b0e4(char *s) {
	char buf[24];
	int i = 0;
	char c;
	char *p = buf;

	goto test;
inc:
	i++;
test:
	c = *s;

	if (!c)
		goto done;

	p[i] = c;
	s++;
	goto inc;
done:
	do {
		p[i++] = '.';
		p[i++] = 'V';
		p[i++] = 'A';
		p[i++] = 'B';
		p[i] = 0;
	} while (0);

	DAT_800d1a98 = FUN_8004b188(buf, &pLevelSpecificSFXVABHeader);
}

/* SFX_SpoolInMIDI__FPCc (0x8004b178) */
void SFX_SpoolInMIDI(const char *name) {}

/* FUN_8004b180 (0x8004b180) */
void FUN_8004b180(void) {}

/* FUN_8004b188 (0x8004b188) */
/* Loads a VAB sound bank: reads the file, sends the sample body (after the header and the
   512-byte-per-program tables) to SPU memory, then shrinks the buffer to the header. The SPU
   address goes into the header's size word. Called by FUN_8004b0e4 with "name.VAB". */
int FUN_8004b188(char *name, char **out) {
	SVabHead *vab = (SVabHead *)Mem_NewX(FileIO_Open(name) + 64, 1, 1);
	int head, body, addr;

	FileIO_Load(vab);
	FileIO_Sync();

	head = (vab->NumProgs << 9) + 2592;
	body = (vab->Size - head + 63) & ~63;
	addr = PC_SoundBankLoad((char *)vab + head, body);

	vab->Size = addr;
	Mem_Shrink(vab, head);

	*out = (char *)vab;
	return 1;
}

/* SFX_PlayX__FUisi (0x8004b250) */
/* Plays sound effect id at a volume (clamped to 16384): records it for replays, then
   keys the VAB voice. Each table entry is {voice, note:16 | pitch:16}. A note of 0 means 60
   (middle C). The pitch is scaled by the global percentage SFXLEVEL. */
int SFX_PlayX(ESfx id, short vol, int pan) {
	int p, note, v, i;

	if ((int)id < 0 || id == 0xffff)
		return 0;

	if (vol > 16384)
		vol = 16384;

	i = id * 2;

	if (DAT_800b8a9c[i] >= 0)
		Video_SFXPlay(id, vol, pan);

	p = DAT_800b8a9c[i + 1];
	note = 60;

	if (p & 0xffff0000) {
		note = p >> 16;
		p &= 0xffff;
	}

	SFX_v = p;
	p = SFXLEVEL * p / 100;
	v = (vol * p * 4) >> 16;

	return playSFX(DAT_800b8a9c[i], note, v, v, pan);
}

/* SFX_PlayLR__FUissi (0x8004b364) */
/* Stereo variant of FUN_8004b250: plays sound effect id with separate left/right
   volumes (each clamped to 16384), scaled by the global percentage SFXLEVEL. */
int SFX_PlayLR(ESfx id, short l, short r, int pan) {
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

/* SFX_PlayPos__FUiRC7CVectori (0x8004b464) */
/* Positional variant of FUN_8004b250: left/right volumes come from the spatial
   attenuation at pos (12 bits each, scaled by the global percentage SFXLEVEL). The
   attenuation's sign bit flips the left channel's phase. */
int SFX_PlayPos(ESfx id, const CVector &pos, int pan) {
	unsigned int att;
	int l, r, p, note;

	if ((int)id < 0 || id == 0xffff)
		return 0;

	att = Utils_CalculateSpatialAttenuation(pos, 256, 12000);
	l = ((att & 0xfff) * SFXLEVEL) >> 14;
	r = (((att >> 16) & 0xfff) * SFXLEVEL) >> 14;

	if ((int)att < 0)
		l = -l;

	p = DAT_800b8a9c[id * 2 + 1];
	note = 60;

	if (p & 0xffff0000) {
		note = p >> 16;
		p &= 0xffff;
	}

	SFX_v = p;

	return playSFX(DAT_800b8a9c[id * 2], note, l * p / 100, r * p / 100, pan);
}

/* SFX_GetVoicePitch__FUi (0x8004b590) */
/* With no voice the C source fell off the end (returning the 1000 still in the register): the
   uninitialised `none` keeps that code, where C++'s missing return would be unreachable; the
   expect keeps C's block order. */
int SFX_GetVoicePitch(unsigned v) {
	SpuVoiceAttr a;
	int none;

	if (SFXPaused)
		return 1000;

	if (__builtin_expect(v != 0, 0)) {
		int i = 0;

		while (!(v >> i & 1))
			i++;

		return PC_SoundChannelGetPitch(i);
	}

	return none;
}

/* SFX_SetVoicePitch__FUiUs (0x8004b5d0) */
void SFX_SetVoicePitch(unsigned m, unsigned short p) {
	SpuVoiceAttr a;
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

/* SFX_ModifyPos__FUiRC7CVectori (0x8004b65c) */
void SFX_ModifyPos(unsigned a, const CVector &pos, int b) {
	SpuVoiceAttr s;
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
				s.voice = i;
				s.mask = 3;

				if ((int)l < -16383)
					l = -16383;

				if ((int)l >= 16384)
					l = 16383;

				if ((int)r < -16383)
					r = -16383;

				if ((int)r >= 16384)
					r = 16383;

				s.volume.left = l;
				s.volume.right = r;

				if (b) {
					x = *tab;
					s.mask = 19;
					y = x - (x * b / 8192);

					if (x < y) {
						d = y - x;
						v = x + (x >> 1);

						if (v < d)
							y = v;
					}

					s.pitch = y;
				}

				PC_SoundChannelVolume(i, s.volume.left, s.volume.right);

				if (s.mask & 16)
					PC_SoundChannelPitch(i, s.pitch);
			}

			tab += 1;
		}
	}
}

/* SFX_ModifyVol__FUiii (0x8004b7cc) */
void SFX_ModifyVol(unsigned mask, int a, int b) {
	int i;
	SpuVoiceAttr s;

	if (mask) {
		if (!(a < 16384))
			a = 16383;

		if (!(b < 16384))
			b = 16383;

		a = (a * SFXLEVEL) >> 14;
		b = (b * SFXLEVEL) >> 14;

		if (a < -16383)
			a = -16383;

		if (!(a < 16384))
			a = 16383;

		if (b < -16383)
			b = -16383;

		if (!(b < 16384))
			b = 16383;

		for (i = 0; i < 24; i++) {
			if (mask & (1 << i)) {
				s.voice = i;
				s.mask = 3;
				s.volume.left = a;
				s.volume.right = b;
				PC_SoundChannelVolume(i, s.volume.left, s.volume.right);
			}
		}
	}
}

/* FUN_8004b8e0 (0x8004b8e0) */
void FUN_8004b8e0(void) {}

/* FUN_8004b8e8 (0x8004b8e8) */
void FUN_8004b8e8(void) {
	SFXPaused = 0;
}

/* SFX_Pause__Fv (0x8004b8f4) */
void SFX_Pause(void) {
	SpuVoiceAttr s;
	int i;
	unsigned short *q;

	if (!SFXPaused) {
		i = 0;
		q = pitchPriorToPause;
		SFXPaused = 1;
		s.mask = 16;
		s.pitch = 0;

		for (; i < 24; i++) {
			*q = (unsigned short)PC_SoundChannelGetPitch(i);
			PC_SoundChannelPitch(i, 0);

			q++;
		}

		PC_StreamMute(1);
	}
}

/* SFX_Unpause__Fv (0x8004b974) */
void SFX_Unpause(void) {
	SpuVoiceAttr s;
	int i;
	unsigned short *q;

	if (SFXPaused) {
		i = 0;
		q = pitchPriorToPause;
		SFXPaused = 0;
		s.mask = 16;

		for (; i < 24; i++) {
			s.pitch = *q;
			PC_SoundChannelPitch(i, (short)s.pitch);

			q++;
		}

		PC_StreamMute(0);
	}
}

/* SFX_Stop__FUi (0x8004b9ec) */
void SFX_Stop(unsigned m) {
	unsigned inv = ~m;

	nko_flag &= inv;
	loop_flag &= inv;
	voiceon_flag &= inv;
	keyon_queue &= inv;
	keyoff_queue |= m;
}

/* FUN_8004ba34 (0x8004ba34) */
void FUN_8004ba34(void) {
	PC_SoundKeyOff(0xffffffff);

	nko_flag = 0;
	keyon_queue = 0;
	keyoff_queue = 0;
	voiceon_flag = 0;
	exp_flag = 0;
	loop_flag = 0;
	voicefree_flag = 0xffffff;
	reverb_changed = 0;
}

/* SFX_SetReverbType__FUc (0x8004ba7c) */
void SFX_SetReverbType(unsigned char t) {
	SpuReverbAttr a;

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

/* SFX_ReverbOff__Fv (0x8004bafc) */
void SFX_ReverbOff(void) {
	SFX_SetReverbType(0);
}

/* SFX_SetReverbDepth__FUcUc (0x8004bb1c) */
void SFX_SetReverbDepth(unsigned char l, unsigned char r) {
	SpuReverbAttr a;

	a.mask = 6;
	a.depth.left = l << 8;
	a.depth.right = r << 8;
	PC_SoundReverbDepth(a.depth.left, a.depth.right);
}

/* SFX_MIDIFadeOut__Fi (0x8004bb54) */
void SFX_MIDIFadeOut(int speed) {}

/* FUN_8004bb5c (0x8004bb5c) */
void FUN_8004bb5c(void) {}

/* SFX_MIDIFadeIn__Fi (0x8004bb64) */
void SFX_MIDIFadeIn(int speed) {}

/* SFX_MIDIChannelMute__FUs (0x8004bb6c) */
void SFX_MIDIChannelMute(unsigned short mask) {}

/* playSFX__FUiUcssi (0x8004bb74) */
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
		Rnd(pitch_offset * 2);
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

/* allocateVoice__Fv (0x8004bef4) */
int allocateVoice(void) {
	int i;

	for (i = 0; i < 24; i++) {
		if (voicefree_flag & (1 << i))
			return i;
	}

	return -1;
}

/* SFX_ProcessingPerFrame__Fv (0x8004bf30) */
void SFX_ProcessingPerFrame(void) {
	short st[4];
	int i;
	unsigned m;

	voicefree_flag = 0;

	if (keyon_queue) {
		PC_SoundKeyOn(keyon_queue);

		keyon_queue = 0;
	}

	if (keyoff_queue) {
		PC_SoundKeyOff(keyoff_queue);

		keyoff_queue = 0;
	}

	if (reverb_changed) {
		PC_SoundReverbChannels(0, 0xffffff);
		PC_SoundReverbChannels(1, reverb_queue);

		reverb_changed = 0;
	}

	for (i = 0; i < 24; i++) {
		m = 1 << i;

		if (nko_flag & m)
			continue;

		if (loop_flag & m)
			continue;

		st[0] = (short)PC_SoundChannelLevel(i);

		if (voiceon_flag & m) {
			if (st[0])
				continue;

			if (exp_flag & m)
				voiceon_flag &= ~m;
			else
				exp_flag |= m;
		} else {
			if (st[0] == 0)
				voicefree_flag |= m;
		}
	}

	nko_flag = 0;
}

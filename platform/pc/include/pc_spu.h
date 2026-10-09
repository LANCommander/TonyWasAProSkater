/* Sound: the SPU (512 KB sound RAM, 24 ADPCM voices, master volume) and its mixer on an SDL audio
 * device. The libspu/libsnd entry points the game calls are implemented in src/pc_spu.c. */
#ifndef PC_SPU_H
#define PC_SPU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PC_SPU_VOICES 24
#define PC_SPU_RAM_SIZE 0x80000u

/* Open the audio device and start the mixer. THPS_AUDIO=0 keeps the SPU model but plays nothing;
 * THPS_AUDIO_DUMP=path also writes the mixed output as raw 44.1 kHz s16le stereo. */
void PC_SpuInit(void);

/* Power-on state (what _SpuInit leaves behind): voices off, sound RAM cleared, allocator reset. */
void PC_SpuReset(void);

/* CD audio input (decoded XA). Queue stereo PCM at `rate` Hz; the mixer resamples it and adds it
 * at the CD volume (SpuSetCommonAttr) through the CD mixer (CdMix). Flush drops what is queued
 * (a seek or a stopped stream). */
void PC_SpuCdQueue(const int16_t *stereo, int frames, int rate);
void PC_SpuCdFlush(void);
/* CdMix levels: CD left to SPU left/right, CD right to SPU right/left; 0x80 = 100%. */
void PC_SpuCdSetMix(const uint8_t atv[4]);
/* CdlMute / CdlDemute. */
void PC_SpuCdMute(int mute);

#ifdef __cplusplus
}
#endif

#endif

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
/* Frames queued and not yet played; frames the mixer has taken from the queue so far (a clock
 * for what has been heard, at the stream's rate). */
unsigned PC_SpuCdQueued(void);
uint64_t PC_SpuCdConsumed(void);
/* CdMix levels: CD left to SPU left/right, CD right to SPU right/left; 0x80 = 100%. */
void PC_SpuCdSetMix(const uint8_t atv[4]);
/* CdlMute / CdlDemute. */
void PC_SpuCdMute(int mute);

/* PCM samples (the Dreamcast's sound banks, pc_dcsound.h). A sample registered here is put in
 * sound RAM as a 32-byte stand-in (PC_SPU_PCM_STUB bytes from PC_SpuPcmStub); a voice keyed on at
 * a stand-in plays the PCM instead, at the voice's pitch and volume, with its envelope. At pitch
 * 0x1000 the sample plays at `rate` x PC_SPU_PCM_TUNE (see pc_spu.c). loop: repeat
 * [loopStart, loopEnd) frames after the end (loopEnd 0 = the end). Returns the sample's id, or
 * -1 if the registry is full. Samples stay registered: the banks are few and reloaded often. */
#define PC_SPU_PCM_STUB 32
int PC_SpuPcmAdd(const int16_t *pcm, uint32_t frames, int rate, int loop, uint32_t loopStart,
                 uint32_t loopEnd);
void PC_SpuPcmStub(int id, uint8_t out[PC_SPU_PCM_STUB]);
/* 1 if sample id loops. */
int PC_SpuPcmLooped(int id);

#ifdef __cplusplus
}
#endif

#endif

/* Libraries that are inert in this port: SPU reverb (the SPU itself is in pc_spu.c). Each returns
 * what the game expects from a working-but-idle device. */
#include <stdint.h>

/* ---- SPU reverb / libsnd: not modelled yet (pc_spu.c has the voices and the mixer) ----------- */

int SpuSetReverbModeParam(void *attr) { (void)attr; return 0; }
int SpuSetReverbVoice(int on, uint32_t voices) { (void)on; return (int)voices; }
int SpuSetReverb(int on) { return on; }
int SpuClearReverbWorkArea(int mode) { (void)mode; return 0; }
void SsUtReverbOff(void) {}

/* Libraries that are inert in this port: host-PC file I/O (LIBSN), the MDEC (no movies), and SPU
 * reverb (the SPU itself is in pc_spu.c). Each returns what the game expects from a
 * working-but-idle device. The memory card is in hle_libcard.c. */
#include <stdint.h>
#include <string.h>

#include "pc_hle.h"
#include "pc_runtime.h"

/* ---- SPU reverb / libsnd: not modelled yet (pc_spu.c has the voices and the mixer) ----------- */

int SpuSetReverbModeParam(void *attr) { (void)attr; return 0; }
int SpuSetReverbVoice(int on, uint32_t voices) { (void)on; return (int)voices; }
int SpuSetReverb(int on) { return on; }
int SpuClearReverbWorkArea(int mode) { (void)mode; return 0; }
void SsUtReverbOff(void) {}

/* ---- LIBSN host I/O (development PC link): absent -------------------------------------------- */

int PCinit(void) { return -1; }
int PCopen(const char *name, int flags, int perms) { (void)name; (void)flags; (void)perms; return -1; }
int PCcreat(const char *name, int perms) { (void)name; (void)perms; return -1; }
int PCread(int fd, void *buf, int len) { (void)fd; (void)buf; (void)len; return -1; }
int PCwrite(int fd, void *buf, int len) { (void)fd; (void)buf; (void)len; return -1; }
int PClseek(int fd, int off, int mode) { (void)fd; (void)off; (void)mode; return -1; }
int PCclose(int fd) { (void)fd; return -1; }

/* ---- MDEC (libpress): movies are skipped ------------------------------------------------------ */

void DecDCTReset(int mode) { (void)mode; }
void DecDCTin(uint32_t buf, int mode) { (void)buf; (void)mode; }
void DecDCTout(uint32_t buf, int size) { (void)buf; (void)size; }
uint32_t DecDCToutCallback(uint32_t func) { (void)func; return 0; }
int hle_8009033c(int mode) { (void)mode; return 0; } /* DecDCTinSync */

/* ---- misc libapi/libetc entry points the game calls directly ---------------------------------- */

int hle_EnterCriticalSection(void) { return 1; }
void hle_ExitCriticalSection(void) {}
void hle_FlushCache(void) {}
uint32_t hle_GPU_cw(uint32_t w) { return w; }

/* libgte: _patch_gte installs the GTE into the BIOS exception handler; nothing to do. */
void hle__patch_gte(void) {}

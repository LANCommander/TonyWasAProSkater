/* Libraries that are inert in this port: the memory card (no card inserted), host-PC file I/O
 * (LIBSN), the MDEC (no movies), and SPU reverb (the SPU itself is in pc_spu.c). Each returns what
 * the game expects from a working-but-idle device. */
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

/* ---- memory card (libcard + libapi card events): no card in either slot -------------------- */

#define HwCARD 0xf0000011u
#define SwCARD 0xf4000001u
#define EvSpIOE 0x0004
#define EvSpERROR 0x8000
#define EvSpTIMOUT 0x0100
#define EvSpNEW 0x2000

void InitCARD(int pad) { (void)pad; }
void InitCARD2(int pad) { (void)pad; }
int StartCARD(void) { return 1; }
int StartCARD2(void) { return 1; }
int StopCARD(void) { return 1; }
void hle__bu_init(void) {}
int hle__card_info(int chan) {
    (void)chan;
    PC_DeliverEvent(SwCARD, EvSpTIMOUT);
    PC_DeliverEvent(HwCARD, EvSpTIMOUT);
    return 1;
}
int hle__card_load(int chan) { return hle__card_info(chan); }
int hle__card_clear(int chan) { return hle__card_info(chan); }
int hle__new_card(void) { return 1; }
int hle__card_write(int chan, int block, uint32_t buf) {
    (void)block; (void)buf;
    return hle__card_info(chan);
}

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

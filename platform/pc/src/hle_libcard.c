/* libcard: inert. There's no memory card on PC: the game's card layer (decomp/src/card, PC
 * branches) keeps careers and replays as files (pc_saves.c) and reports itself always ready, so
 * nothing reaches the card hardware or the BIOS file API on "bu00:". The game's card setup
 * (FUN_80014d5c) still calls these. */
#include <stdint.h>

#include "pc_hle.h"
#include "pc_runtime.h"

void InitCARD(int pad) { (void)pad; }
void InitCARD2(int pad) { (void)pad; }
int StartCARD(void) { return 1; }
int StartCARD2(void) { return 1; }
int StopCARD(void) { return 1; }
void hle__bu_init(void) {}

int hle__card_info(int chan) { (void)chan; return 1; }
int hle__card_load(int chan) { (void)chan; return 1; }
int hle__card_clear(int chan) { (void)chan; return 1; }
int hle__new_card(void) { return 1; }
int hle__card_write(int chan, int block, uint32_t buf) {
    (void)chan; (void)block; (void)buf;
    return 1;
}

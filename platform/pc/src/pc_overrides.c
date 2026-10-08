/* Decompiled game functions replaced on PC (hleov_<addr> beats the decompiled C).
 * Keep this list short: each entry is a feature the port doesn't support yet. */
#include <stdlib.h>

#include "pc_runtime.h"

/* GameFMV_PlayMovie(int): STR movie playback needs CD streaming + MDEC, not implemented yet.
 * Skipping a movie is what happens when the player presses a button through it. Set
 * THPS_MOVIES=1 to run the original code anyway. */
void hleov_80022c1c(int track) {
    static void (*orig)(int);
    if (getenv("THPS_MOVIES") && atoi(getenv("THPS_MOVIES"))) {
        extern void pcfn_80022c1c(int);
        pcfn_80022c1c(track);
        return;
    }
    PC_Log("movie %d skipped", track);
    (void)orig;
}

/* The Dreamcast version's sound banks (.kat) as the PS1 game's VABs (pc_dcsound.cpp; the C
 * interface is pc_dcsound.h). */
#pragma once

#include <stdint.h>


namespace pc {

class DcSound {
public:
    /* A VAB for the game to load in place of `vab` ("ware.vab"), made from the .kat of the same
     * name in Data/: its samples are registered as PCM (pc/sound.h) and the VAB's samples are
     * their stand-ins. Returns a malloc'd image and its size, or nullptr if there is no such .kat. */
    uint8_t *Vab(const char *vab, int *size);

    /* Point the game's sound effects at the .kat sounds the Dreamcast version plays for them
     * (some moved, so the PS1 table would play the wrong ones), at its volumes. Once, before the
     * game runs. */
    static void Table();

private:
    static constexpr int kProgsMax = 128;   /* a VAB's programs */
    static constexpr int kTones = 16;       /* tones per program */
    static constexpr int kMaxBanks = 32;

    struct Bank {
        char name[64] = {};                 /* the .kat's file name */
        int tone[kProgsMax * kTones] = {};  /* PCM sample id + 1 (0: none) */
    };

    Bank *LoadBank(const char *kat);

    Bank m_banks[kMaxBanks];
    int m_nbanks = 0;
};

extern DcSound g_dcSound;

} // namespace pc

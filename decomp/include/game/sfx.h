/* Sound effects (SFX.CPP; the PAL module in functions.tsv): playing, positioning and
   stopping VAB voices. */
#ifndef GAME_SFX_H
#define GAME_SFX_H

#include "game/types.h"
#include "game/vector.h"

/* functions */
class CBruce;

void SFX_Grunt(CBruce *p);                                                /* 0x80043408 */
void SFX_Init(char *);                                                    /* 0x8004b09c */
int SFX_PlayX(uint id, short vol, int pan);                               /* 0x8004b250 */
int SFX_PlayLR(uint id, short l, short r, int pan);                       /* 0x8004b364 */
int SFX_PlayPos(uint id, const CVector &pos, int pan);                    /* 0x8004b464 */
void SFX_Stop(uint m);                                                    /* 0x8004b9ec */
int playSFX(uint sfx, uchar pitch, short vl, short vr, int pitch_offset); /* 0x8004bb74 */

#endif

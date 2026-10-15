/* Sound effects (SFX.CPP; the PAL module in functions.tsv): playing, positioning and
   stopping VAB voices. */
#ifndef GAME_SFX_H
#define GAME_SFX_H

#include "game/types.h"
#include "game/vector.h"

/* functions */
class CBruce;
void SFX_InitAtStart();
void SFX_Init(char *a);
void FUN_8004b0e4(char *s);
void SFX_SpoolInMIDI(const char *name);
void FUN_8004b180();
int SFX_PlayX(uint id, short vol, int pan);
int SFX_GetVoicePitch(unsigned v);
void SFX_SetVoicePitch(unsigned m, unsigned short p);
void SFX_ModifyPos(unsigned a, const CVector &pos, int b);
void SFX_ModifyVol(unsigned mask, int a, int b);
void FUN_8004b8e0();
void FUN_8004b8e8();
void SFX_Pause();
void SFX_Unpause();
void SFX_Stop(unsigned m);
void FUN_8004ba34();
void SFX_SetReverbType(unsigned char t);
void SFX_ReverbOff();
void SFX_SetReverbDepth(unsigned char l, unsigned char r);
void SFX_MIDIFadeOut(int speed);
void FUN_8004bb5c();
void SFX_MIDIFadeIn(int speed);
void SFX_MIDIChannelMute(unsigned short mask);
void SFX_ProcessingPerFrame();

int SFX_PlayLR(uint id, short l, short r, int pan);
int SFX_PlayPos(uint id, const CVector &pos, int pan);
int playSFX(uint sfx, uchar pitch, short vl, short vr, int pitch_offset);

/* globals */
extern uint SFX_ReverbDepthSetTimer;
#endif

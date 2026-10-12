/* Career_Got__Fi (0x800106e0) */
/* C++ (CAREER.CPP), next to Career_GiveTape. Whether the current skater has goal (tape) `Goal` of
   the current level. */
#include "game/career.h"
#include "game/bruce.h"

extern CBruce *GSkater;
extern SCareer GCareer;
extern int GLevel;

extern "C" int Career_Got__Fi(int Goal) {
	return (GCareer.CharacterProgress[GSkater->mSkaterIndex].Tapes[GLevel] >> Goal) & 1;
}

/* FUN_8003159c (0x8003159c) */
/* C++ (PHYSICS.CPP). Called by HandleStickToRail when a trick is in progress (m35C): unless already
   scored (m364), plays the landing sound for anims 62..66 and scores the held trick (m368 & 0xff,
   0x4000/0x2000 states, the same test as the Limit block there) via Trick_Trick from the 44-byte
   trick table, counting it in m424, then sets 0x800 (the 0x4000 state always, the 0x2000 state only when the trick
   has frames; retail's `j L66` is the cross-jumped `m368 |= 0x800` tail).
   Score is declared at function scope and assigned in both arms: a pseudo set twice loses
   sched1's set-once priority boost, so its load stays above the m424 increment as retail has it.) */
#include "game/bruce.h"

extern STrick Group[];

int SFX_PlayX(unsigned int, short, int);

extern "C" void FUN_8003159c(CBruce *pBruce) {
	if (pBruce->m364)
		return;

	if ((ushort)(pBruce->mAnim - 62) < 5)
		SFX_PlayX(162, 0x2000, 0);

	int State = pBruce->m368;
	int Trick = State & 0xff;
	int Score;

	if (State & 0x4000) {
		Score = Group[Trick].Score;
		pBruce->m424++;
		pBruce->Trick_Trick(Trick, Group[Trick].pName, Score, 0);

		pBruce->m368 |= 0x800;
	} else if (State & 0x2000) {
		Score = Group[Trick].Score;
		pBruce->m424++;
		pBruce->Trick_Trick(Trick, Group[Trick].pName, Score, 0);

		if (Group[Trick].Frames)
			pBruce->m368 |= 0x800;
	}
}

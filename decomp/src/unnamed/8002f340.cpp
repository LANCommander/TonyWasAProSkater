/* Trick_Trick__6CBruceiPcii (0x8002f340) */
/* C++ (PHYSICS.CPP). Scores a trick: counts it, depreciates its score by how often it was done (a
   negative score is taken as-is), zeroes it on a rail in some mode, and adds it to the panel. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

extern int GGame;

extern "C" int FUN_8002f290(CBruce *, int, int);
extern "C" void FUN_8006e19c(CBruce *, char *, int, int);

void CBruce::Trick_Trick(int Trick, char *pName, int Score, int) {
	if (m760) {
		if (GGame == 7)
			mTime = 0;

		m428[Trick]++;
		m568[Trick]++;

		if (Score < 0)
			Score = -Score;
		else
			Score = FUN_8002f290(this, Trick, Score);

		if (m34C == 4) {
			m428[Trick]--;
			Score = 0;
			m568[Trick]--;
		}

		FUN_8006e19c(this, pName, Score, 0);
	}
}

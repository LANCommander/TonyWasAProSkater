/* GetStats__6CBrucei (0x80041ce0) */
/* C++ (MECH.CPP). Demo twin; locals from its MAIN.SYM. Fills player Skater's four stat bytes: in a
   career game, from the best board the skater's tape count has unlocked; otherwise all 3. */
#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];
extern SCareer GCareer;
extern BoardStats GBoardStats[];
extern int GNumberOfPlayers;

void CBruce::GetStats(int Skater) {
	if (GNumberOfPlayers == 1) {
		int highest_board = 0;

		while (highest_board < 7) {
			if (GCareer.CharacterProgress[mSkaterIndex].NumTapes <
			    Stats[mSkaterIndex].Boards[highest_board + 1].Req)
				break;

			highest_board++;
		}

		GBoardStats[Skater].s[0] = Stats[mSkaterIndex].Boards[highest_board].Stats[0];
		GBoardStats[Skater].s[1] = Stats[mSkaterIndex].Boards[highest_board].Stats[1];
		GBoardStats[Skater].s[2] = Stats[mSkaterIndex].Boards[highest_board].Stats[2];
		GBoardStats[Skater].s[3] = Stats[mSkaterIndex].Boards[highest_board].Stats[3];
	} else {
		GBoardStats[Skater].s[0] = 3;
		GBoardStats[Skater].s[1] = 3;
		GBoardStats[Skater].s[2] = 3;
		GBoardStats[Skater].s[3] = 3;
	}
}

/* DoFootPlantPhysics__6CBruce (0x80036f7c) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Demo twin (no locals in MAIN.SYM). Foot plant: fires the trick, sets the
   plant timer from the skater's stats, flips the velocity back along -mFront (keeping speed),
   nudges it forward and kicks the skater upward by a stat-derived amount.
   GetStat is a macro, not an inline: gcc 2.x fold() distributes the enclosing arithmetic into the
   ?: arms (and hoists the save_expr'd operands), which is what produces retail's jump-threaded
   override tests and the per-arm sums. Operand order (Stats + (flag ? 3 : 0)) + Bonus fixes the
   load order (base, then bonus byte). */
#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];

int Cheat_StatsOverride; /* stats cheat override */

extern int Cheat_Fire;
extern char s_FOOTPLANT[];

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride                                                                           \
	     ? Cheat_StatsOverride                                                                     \
		 : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +              \
		       mpStatBonus[n])

void CBruce::DoFootPlantPhysics() {
	Trick_Trick(65, s_FOOTPLANT, 100, 0);

	mJumpStrength = 15 - (GetStat(0) + GetStat(2)) / 2;
	StateTransition(PHYSICS_IN_AIR, 5587);

	m740 = 1;
	mVel = -mFront * mVel.Length();
	mVel.vx += mFront.vx * 5;
	mVel.vz += mFront.vz * 5;
	mVel.vy -=
	    (mJumpStrength * (((32 - (10 - GetStat(0)) / 2) + ((10 - GetStat(0)) / 3 - 18)) << 10) /
		     (15 - (GetStat(0) + GetStat(2)) / 2) +
		 ((32 - (10 - GetStat(0)) / 2) << 10)) *
	    3;
}

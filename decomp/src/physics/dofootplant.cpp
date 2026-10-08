/* DoFootPlantPhysics__6CBruce (0x80036f7c) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Demo twin (no locals in MAIN.SYM). Foot plant: fires the trick, sets the
   plant timer from the skater's stats, flips the velocity back along -mFront (keeping speed),
   nudges it forward and kicks the skater upward by a stat-derived amount.
   GetStat is a macro, not an inline: gcc 2.x fold() distributes the enclosing arithmetic into the
   ?: arms (and hoists the save_expr'd operands), which is what produces retail's jump-threaded
   override tests and the per-arm sums. Operand order (Stats + (flag ? 3 : 0)) + Bonus fixes the
   load order (base, then bonus byte). */
struct CVector {
	int vx, vy, vz;
	int Length() const;
	CVector operator-() const;
};

CVector operator*(const CVector &, const int &);

struct SSkater {
	char pad0[0x18];
	int Stats[10]; /* 0x18 */
	char pad40[340 - 0x40];
};

extern SSkater DAT_800a44fc[];

int Cheat_StatsOverride; /* stats cheat override */

extern int Cheat_Fire;
extern char s_FOOTPLANT[];

class CBruce {
  public:
	char pad0[0x6c];
	CVector mVel; /* 0x6c */
	char pad78[0x198 - 0x78];
	unsigned char *mpStatBonus; /* 0x198 */
	char pad19c[0x254 - 0x19c];
	int m254; /* 0x254 */
	char pad258[0x2e8 - 0x258];
	int mSkaterIndex; /* 0x2e8 */
	char pad2ec[0x740 - 0x2ec];
	int m740; /* 0x740 */
	int m744;
	int m748; /* 0x748 */
	char pad74c[0x7d8 - 0x74c];
	CVector mFront; /* 0x7d8 */
	void Trick_Trick(int, char *, int, int);
	void StateTransition(int, int);
	void DoFootPlantPhysics();
};

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride ? Cheat_StatsOverride                                                                   \
	              : (DAT_800a44fc[mSkaterIndex].Stats[n] + ((m254 || Cheat_Fire) ? 3 : 0)) +     \
	                    mpStatBonus[n])

void CBruce::DoFootPlantPhysics() {
	Trick_Trick(65, s_FOOTPLANT, 100, 0);

	m748 = 15 - (GetStat(0) + GetStat(2)) / 2;
	StateTransition(1, 5587);

	m740 = 1;
	mVel = -mFront * mVel.Length();
	mVel.vx += mFront.vx * 5;
	mVel.vz += mFront.vz * 5;
	mVel.vy -= (m748 * (((32 - (10 - GetStat(0)) / 2) + ((10 - GetStat(0)) / 3 - 18)) << 10) /
	                (15 - (GetStat(0) + GetStat(2)) / 2) +
	            ((32 - (10 - GetStat(0)) / 2) << 10)) *
	           3;
}

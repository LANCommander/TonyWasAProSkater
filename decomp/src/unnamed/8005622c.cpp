/* FUN_8005622c (0x8005622c) flags=-fstructure-noalias */
/* C++ (credits). Two-player sibling of FUN_8005933c: new CBruce for player Player with the
   skater index from DAT_800d2304[Player], a per-player x position, and the stance flag. Lever:
   -fstructure-noalias (the index re-reads come from the separate array expressions). */
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

struct MATRIX {
	short m[3][3];
	int t[3];
};

struct CVector {
	int vx, vy, vz;
	void Set(int x, int y, int z) {
		vx = x;
		vy = y;
		vz = z;
	}
};

struct SStats {
	char pad0[0x8];
	char *pName; /* 0x08 */
	int Style;   /* 0x0c */
	char pad10[0x18 - 0x10];
	int Stats[10]; /* 0x18 */
	char *pStance; /* 0x40 */
	char pad44[340 - 0x44];
};

extern SStats Stats[];
extern char DAT_800be444[];

int DAT_800d2304[2];
int DAT_800d2300;
int DAT_800d22fc;
int DAT_800d22f8;

class CBruce {
  public:
	char pad0[0x4];
	CVector mPos; /* 0x004 */
	char pad10[0x20 - 0x10];
	int m20; /* 0x020 */
	char pad24[0x3c - 0x24];
	void *mp3C; /* 0x03c */
	char pad40[0x134 - 0x40];
	int mFlags; /* 0x134 */
	char pad138[0x150 - 0x138];
	int m150; /* 0x150 */
	char pad154[0x2e8 - 0x154];
	int mSkaterIndex; /* 0x2e8 */
	char pad2ec[0x78c - 0x2ec];
	MATRIX mRotMatrix; /* 0x78c */
	char pad7ac[3612 - 0x7ac];
	void *operator new(uint) throw();
	CBruce();
	void InitItem(char *);
	void CycleAnim(int, signed char);
};

extern "C" void FUN_8008b238(MATRIX *);

extern "C" CBruce *FUN_8005622c(int Player) {
	CBruce *pBruce = new CBruce;
	pBruce->mSkaterIndex = DAT_800d2304[Player];

	pBruce->InitItem(Stats[DAT_800d2304[Player]].pName);

	pBruce->m150 = 0x10000;

	pBruce->CycleAnim(0, 1);

	pBruce->m20 = 0x404040;

	if (!Player)
		pBruce->mPos.Set(DAT_800d2300, DAT_800d22f8, 0x1bc000);
	else
		pBruce->mPos.Set(DAT_800d22fc, DAT_800d22f8, 0x1bc000);
	FUN_8008b238(&pBruce->mRotMatrix);

	if (*Stats[DAT_800d2304[Player]].pStance == 'R' ||
	    *Stats[DAT_800d2304[Player]].pStance == 'r')
		pBruce->mFlags |= 2;
	pBruce->mp3C = DAT_800be444;
	return pBruce;
}

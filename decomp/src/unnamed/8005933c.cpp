/* FUN_8005933c (0x8005933c) flags=-fstructure-noalias */
/* C++ (credits). Creates the skater shown in the credits: new CBruce, model of the selected skater
   (or skater 0 if that slot has no name), idle anim, fixed position, identity rotation, and the
   goofy flag from the stance string ('R'/'r'). Lever: -fstructure-noalias keeps the selected index
   in a register across the mSkaterIndex store. */
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

int DAT_800d1c40;
int DAT_800d232c;
int DAT_800d2328;

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

extern "C" CBruce *FUN_8005933c() {
	CBruce *pBruce = new CBruce;

	if (!*Stats[DAT_800d1c40].pName) {
		pBruce->mSkaterIndex = 0;
		pBruce->InitItem(Stats[0].pName);
	} else {
		pBruce->mSkaterIndex = DAT_800d1c40;
		pBruce->InitItem(Stats[DAT_800d1c40].pName);
	}

	pBruce->m150 = 0x10000;

	pBruce->CycleAnim(0, 1);

	pBruce->m20 = 0x404040;

	pBruce->mPos.Set(DAT_800d232c, DAT_800d2328, 0xe6000);
	FUN_8008b238(&pBruce->mRotMatrix);

	if (*Stats[DAT_800d1c40].pStance == 'R' || *Stats[DAT_800d1c40].pStance == 'r')
		pBruce->mFlags |= 2;
	pBruce->mp3C = DAT_800be444;
	return pBruce;
}

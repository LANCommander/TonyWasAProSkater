/* Trick_Land__6CBruce (0x8002f478) */
/* C++ (PHYSICS.CPP). Fuzzy demo twin Trick_Land (demo PHYSICS.CPP:605). The skater lands a trick:
   count the landing, score the spin as an "N OLLIE" (THPS1 names each spin amount where the demo
   computes -(n*50)), refresh the panel, fire the pending item, and feed the crowd meter into the
   special meter. */
typedef unsigned short ushort;
typedef unsigned char uchar;
struct Item;

extern int GGame;
extern char s_180_OLLIE[], s_360_OLLIE[], s_540_OLLIE[], s_720_OLLIE[], s_900_OLLIE[],
    s_1080_OLLIE[], s_MEGA_OLLIE[], s_Setting_Playtime_to_0[];

class CBruce {
  public:
	char pad0[0x234];
	int m234; /* 0x234 */
	char pad238[0x250 - 0x238];
	int m250; /* 0x250 */
	int m254; /* 0x254 */
	char pad258[0x284 - 0x258];
	int m284; /* 0x284 */
	int m288; /* 0x288 */
	char pad28c[0x328 - 0x28c];
	int m328; /* 0x328 */
	int m32C; /* 0x32c */
	int m330; /* 0x330 */
	char pad334[0x364 - 0x334];
	int m364; /* 0x364 */
	int m368; /* 0x368 */
	char pad36c[0x418 - 0x36c];
	Item *m418;  /* 0x418 */
	int m41C[2]; /* 0x41c */
	char pad424[0x720 - 0x424];
	int m720; /* 0x720 */
	char pad724[0x738 - 0x724];
	int m738; /* 0x738 */
	char pad73c[0x760 - 0x73c];
	int m760; /* 0x760 */
	char pad764[0x924 - 0x764];
	char mPanel[0xa64 - 0x924]; /* 0x924 */
	unsigned mA64;              /* 0xa64 */
	void Trick_Trick(int, char *, int, int);
	void Trick_Land();
};

extern "C" {
	void TrickMessageRefresh__10CPanelData(char *);
	void FUN_8006e20c(CBruce *);
	int FUN_8004d83c(void *);
	int FUN_8004f000(Item *, CBruce *, int);
	void FUN_80010d84(int);
	void FUN_80042700(CBruce *, int, int, int, int);
	void FUN_8008c190(char *);
}

void CBruce::Trick_Land() {
	m234++;
	m720 = 0;

	if (!m288) {
		int Spin = m328;

		if (((Spin ^ (Spin >> 31)) - (Spin >> 31)) > 1500)
			m32C++;
	}

	if (m32C == 1 && m738 == 2)
		m32C = 0;

	TrickMessageRefresh__10CPanelData(mPanel);

	m288 = 0;
	m284 = 0;

	if (!m364 && m32C) {
		if (m738 != 2 || m32C >= 2) {
			int n = m32C;

			m32C = 0;

			switch (n) {
				case 1:
					Trick_Trick(74, s_180_OLLIE, -50, 0);
					break;
				case 2:
					Trick_Trick(74, s_360_OLLIE, -100, 0);
					break;
				case 3:
					Trick_Trick(74, s_540_OLLIE, -150, 0);
					break;
				case 4:
					Trick_Trick(74, s_720_OLLIE, -200, 0);
					break;
				case 5:
					Trick_Trick(74, s_900_OLLIE, -250, 0);
					break;
				case 6:
					Trick_Trick(74, s_1080_OLLIE, -300, 0);
					break;
				case 7:
					Trick_Trick(74, s_MEGA_OLLIE, -350, 0);
					break;
			}

			m364 = 1;
		}
	}

	FUN_8006e20c(this);

	if (m364) {
		if (m418 && FUN_8004d83c(m41C)) {
			FUN_8004f000(m418, this, mA64);

			m418 = 0;
		}

		if (m364 && GGame == 7) {
			FUN_8008c190(s_Setting_Playtime_to_0);

			m330 = 0;
		}
	}

	FUN_80010d84(mA64);
	unsigned Cheer = mA64;

	if (m364) {
		if (!m254) {
			m250 += Cheer;

			if (m250 >= 3000) {
				m250 = 0;
				m254 = 750;
				FUN_80042700(this, 20, 20, 20, 5);
			}
		} else {
			m254 += Cheer / 5;

			if (m254 > 750)
				m254 = 750;
		}
	}

	m364 = 0;
	m368 = 0;
	m32C = 0;
	m328 = 0;
	m760 = 0;
}

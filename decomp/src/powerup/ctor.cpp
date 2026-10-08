/* __8CPowerUpUsRC7CVectorT2Uii (0x8001f934) */
/* C++ (POWERUP.CPP). Fuzzy demo twin; body from the earlier C draft, as a real constructor so `this`
   is copied where retail copies it. Layout by THPS1 offsets; callees keep their linker names. */
typedef struct {
	int x, y, z;
} V;

class CItem {
  public:
	char pad0[4];
	V v4;
	char pad1[0x16 - 0x10];
	short model;
	char pad2[0x3a - 0x18];
	unsigned short type;
	char pad3[0x48 - 0x3c];
	virtual ~CItem();
};

class CBody : public CItem {
  public:
	CBody();
};

class CPowerUp : public CBody {
  public:
	char pad4[0x6c - 0x4c];
	V v6c;
	char pad5[0x7c - 0x78];
	int f7c;
	char pad6[0x85 - 0x80];
	unsigned char b85;
	char pad7[0x8a - 0x86];
	short f8a;
	char pad8[0xd8 - 0x8c];
	short fd8;
	char pad9[6];
	short fe0;
	char pad10[0x13d - 0xe2];
	unsigned char b13d;
	unsigned char b13e;
	unsigned char b13f;
	unsigned char b140;
	char pad11[0x144 - 0x141];
	int f144;
	V v148;
	char pad12[0x156 - 0x154];
	short f156;
	short f158;
	short f15a;
	short f15c;
	char pad13[2];
	int f160;
	CPowerUp(unsigned short a1, const V &a2, const V &a3, unsigned int a4, int a5);
	virtual ~CPowerUp();
};

extern char s_skmedals[];
extern char s_items_800d161c[];
extern int GGame;
extern CPowerUp *GPowS;
extern CPowerUp *GPowK;
extern CPowerUp *GPowA;
extern CPowerUp *GPowT;
extern CPowerUp *GPowE;
extern CPowerUp *GPowTape;
extern unsigned char CurrentItemsFileRegion;
extern unsigned char CurrentMedalsFileRegion;

char PowerUpList[8];

extern "C" void DontDisplay__8CPowerUp(CPowerUp *);
extern "C" void AttachTo__5CBodyPP5CBody(CPowerUp *, char *);
extern "C" void ShadowOn__5CBody(CPowerUp *);
extern "C" void InitItem__5CItemPc(CPowerUp *, char *);
extern "C" int Redbook_Rnd__Fi(int);
extern "C" short Spool_GetModel__FUii(unsigned, int);
extern "C" int Utils_GetGroundHeight__FRC7CVectorii(V *, int, int);

CPowerUp::CPowerUp(unsigned short a1, const V &a2_, const V &a3_, unsigned int a4, int a5) {
	const V *a2 = &a2_, *a3 = &a3_;

	char *c;

	this->f160 = 0;
	this->type = a1;
	this->b140 = 0;
	this->f15c = a5;
	this->v148 = *a2;
	this->v4 = *a2;

	if (a4 & 8) {
		this->v6c.y = 0xfffc4000;
	} else {
		this->v6c = *a3;
	}

	c = (char *)this + 0x85;
	this->b85 = 5;
	c[1] = 5;
	c[2] = 5;
	this->f8a = 100;
	this->f7c = 0x8000;
	this->f15a = Redbook_Rnd__Fi(20) + 90;
	this->f156 = 50;
	this->f158 = 50;

	if (a4 & 16) {
		this->b13e = 1;
		this->b13f = 1;
	} else if (a4 & 4) {
		this->b13e = 1;
		this->f144 = Utils_GetGroundHeight__FRC7CVectorii(&this->v148, 0, 8000);
	}

	this->fe0 = 300;
	ShadowOn__5CBody(this);

	this->fd8 = 300;
	InitItem__5CItemPc(this, s_items_800d161c);

	switch (this->type) {
		case 0x664:
			InitItem__5CItemPc(this, s_skmedals);

			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x54636518, CurrentMedalsFileRegion);
			break;
		case 0x665:
			InitItem__5CItemPc(this, s_skmedals);

			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0xba6d0434, CurrentMedalsFileRegion);
			break;
		case 0x666:
			InitItem__5CItemPc(this, s_skmedals);

			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x2364558e, CurrentMedalsFileRegion);
			break;
		case 5:
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x311d55d4, CurrentItemsFileRegion);
			break;
		case 4:
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x2328a71c, CurrentItemsFileRegion);
			break;
		case 6:
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x2ebf22ca, CurrentItemsFileRegion);
			break;
		case 15:
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x34524351, CurrentItemsFileRegion);
			break;
		case 10:
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x29b68a16, CurrentItemsFileRegion);
			break;
		case 18:
		case 16:
			this->f8a = 150;
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0xd5108e8c, CurrentItemsFileRegion);
			break;
		case 21:
			this->f8a = 200;
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x6f0f6b9f, CurrentItemsFileRegion);
			break;
		case 22:
			this->f8a = 200;
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0xf6063a25, CurrentItemsFileRegion);
			break;
		case 23:
			this->f8a = 200;
			this->b13d = 1;
			this->model = Spool_GetModel__FUii(0x81010ab3, CurrentItemsFileRegion);
			break;
	}

	if (GGame != 1) {
		switch (this->type) {
			case 4:
			case 5:
			case 6:
			case 10:
			case 15:
			case 16:
				this->f15c = 1;
				break;
		}
	} else {
		switch (this->type) {
			case 5:
				GPowS = this;
				break;
			case 4:
				GPowK = this;
				break;
			case 6:
				GPowA = this;
				break;
			case 15:
				GPowT = this;
				break;
			case 10:
				GPowE = this;
				break;
			case 16:
				GPowTape = this;
				break;
		}
	}

	AttachTo__5CBodyPP5CBody(this, PowerUpList);

	if (this->b13d == 0)
		DontDisplay__8CPowerUp(this);
}

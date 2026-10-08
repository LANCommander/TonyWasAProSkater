/* Competition_ResetMarquee__Fv (0x8007657c) */
typedef unsigned char uchar;

struct Msg {
	int a;
	int b;
	char pad[120 - 8];
};

struct Slot {
	char r10a;
	char r30a;
	char r10b;
	char r30b;
};

struct Comp {
	char p0[4];
	uchar A[10];
	char p1[2];
	int f10, f14, f18;
	char p2[0x2c - 0x1c];
	struct {
		uchar a, b, c, d;
		int pad;
	} S[5];
	char p3[0x5c - 0x54];
	uchar D[10];
	char p4[0x66 - 0x66];
	uchar C[10];
	char p5[0x70 - 0x70];
	short Aa[10][3];
	short B[10];
	char p6[0xc0 - 0xc0];
	uchar flag;
	char p7[0xd4 - 0xc1];
	char *ptr;
	char buf[0x124 - 0xd8];
	int f124, f128, f12c;
};

extern Comp theCompetition;
extern int Vblanks, SelectedSkater;
extern const int GLevel;
extern Msg Levels[];
extern char s_s_s_HEAT_d[];

short HOLYCOW_PLACE, FIRST_PLACE, SECOND_PLACE, THIRD_PLACE, LAST_PLACE;

void Redbook_InitialRand(int);
int Redbook_Rnd(int);
extern "C" void FUN_8008b900(char *, char *, int, int, int);

void Competition_ResetMarquee() {
	Comp *c = &theCompetition;
	int n;

	if (c->flag == 0) {
		c->f10 = 0;
		c->f14 = 0;
		c->f18 = 0;
		c->f12c = 0;
		Redbook_InitialRand(Vblanks);

		{
			int i;

			for (i = 0; i < 10; i++)
				c->A[i] = i;
		}

		n = SelectedSkater;
		c->f128 = n;

		if (n == 10) {
			c->f128 = Redbook_Rnd(10);
			c->A[c->f128] = n;
		}

		if (GLevel == 8) {
			HOLYCOW_PLACE = 975;
			FIRST_PLACE = 935;
			SECOND_PLACE = 910;
			THIRD_PLACE = 860;
			LAST_PLACE = 770;
		} else {
			HOLYCOW_PLACE = 975;
			FIRST_PLACE = 920;
			SECOND_PLACE = 870;
			THIRD_PLACE = 820;
			LAST_PLACE = 520;
		}

		{
			uchar i;

			for (i = 0; i < 5; i++) {
				c->S[i].a = Redbook_Rnd(10);
				c->S[i].c = Redbook_Rnd(10);
				c->S[i].b = Redbook_Rnd(30);
				c->S[i].d = Redbook_Rnd(30);
			}
		}

		{
			uchar i;

			for (i = 0; i < 10; i++) {
				uchar j;

				for (j = 0; j < 3; j++)
					c->Aa[i][j] = 0;

				c->B[i] = 0;
				c->C[i] = i;
			}
		}

		c->flag = 0;

		{
			int i;

			for (i = 0; i < 10; i++)
				c->D[i] = 255;
		}

		{
			int i;

			for (i = 0; i < 10; i++) {
				int k, j, slot;

				slot = Redbook_Rnd(10 - i);
				j = 0;
				k = -1;

				while (slot >= j) {
					k++;

					if (c->D[k] == 255)
						j++;
				}

				c->D[k] = i;
			}
		}

		{
			int i;

			for (i = 0; i < 10; i++)
				if (c->D[i] == 9)
					c->D[i] = c->D[c->f128];
		}

		c->D[c->f128] = 255;
	}

	c->f124 = 0;
	c->ptr = 0;
	FUN_8008b900(c->buf, s_s_s_HEAT_d, Levels[GLevel].a, Levels[GLevel].b,
	             c->flag + 1);

	c->ptr = c->buf;
}

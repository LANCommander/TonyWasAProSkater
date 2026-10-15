/* LevelSelect_DecodePics__Fv (0x800668e8) */
#include "game/classsizes.h"
#include "names.h"

int DAT_800d2364, DAT_800d2378, DAT_800d237c, DAT_800d2384, DAT_800d2388, DAT_800d238c,
    DAT_800d2390;

extern int Pal256X, Pal256Y;
struct SlicedImage;

extern char *DAT_800d3810[];
extern int DAT_800d3840[], DAT_800bf2a0[];
extern struct SlicedImage *DAT_800bf2f0[];

void Load8BitBMP2(char *, char **, int *, int *, unsigned short *);
struct SlicedImage *__nw__6CClassUi(int);
struct SlicedImage *SlicedImage_ctor(struct SlicedImage *, char *, int, int, int, int, int, int);
int Get256SlotNumber(unsigned short);
void MakeGreyClut(int, int);

void LevelSelect_DecodePics(void) {
	unsigned short a;
	int pa;
	char *b;
	int c, d;
	int s1, h, t;
	struct SlicedImage *v;
	struct SlicedImage **ps;

	if (DAT_800d2384 == 0)
		DAT_800d2364 = DAT_800d2378;

	s1 = DAT_800d238c;

	if (s1 == 1 && DAT_800d2388 == 0 && DAT_800d3840[DAT_800d2364]) {
		DAT_800d2390--;
		Load8BitBMP2(DAT_800d3810[DAT_800d2364], &b, &c, &d, &a);

		t = DAT_800d2364;
		DAT_800d3810[t] = 0;
		ps = &DAT_800bf2f0[t];
		v = __nw__6CClassUi(SIZEOF_SLICEDIMAGE);

		if (v)
			v = SlicedImage_ctor(v, b, c, d, 32, 63, 8, a);

		*ps = v;

		if (DAT_800bf2a0[DAT_800d2364] == 0)
			MakeGreyClut(Pal256X, Pal256Y + Get256SlotNumber(a));

		h = DAT_800d237c / 2;

		if (DAT_800d2384 < h)
			DAT_800d2384 = DAT_800d237c - DAT_800d2384 - 1;
		else {
			if (h >= DAT_800d2384) {
				DAT_800d2388 = s1;
				return;
			}

			DAT_800d2384 = DAT_800d237c - DAT_800d2384;
		}

		DAT_800d2364 = DAT_800d2378 + DAT_800d2384;

		if (DAT_800d2364 >= DAT_800d237c)
			DAT_800d2364 -= DAT_800d237c;
	}
}

/* Video_SetHeader__FP10SVideoHeadi (0x80069394) */
#include "names.h"

typedef struct {
	int a, b;
} SVideoMark;

typedef struct {
	int state, f4, f8, fc;
	unsigned *p;
	unsigned buf;
	int n;
	int f1c;
	char pad[0x30];
	SVideoMark marks[14];
} SVideoInfo;

typedef struct {
	int f0;
	SVideoMark marks[5];
	unsigned short f2c, f2e, f30, f32, f34, f36;
} SVideoHead;

typedef struct CBruce {
	char pad[0x1c];
	struct CBruce *other;
	char pad2[0x2e8 - 0x20];
	unsigned short f2e8;
} CBruce;

extern SVideoInfo GVInfo1[];
extern CBruce *MechList;
extern unsigned short GNumberOfPlayers, GLevel, GGame;

void Video_SetHeader(SVideoHead *h, int which) {
	SVideoInfo v;
	int i;
	CBruce *b, *o;

	{
		SVideoInfo *p;

		if (which)
			p = &GVInfo1[1];
		else
			p = &GVInfo1[0];

		v = *p;
	}

	h->f0 = v.f1c;

	for (i = 0; i < 5; i++)
		h->marks[i] = v.marks[i];

	{
		unsigned short t = GNumberOfPlayers;

		b = MechList;
		h->f2c = t;
	}

	o = b->other;

	switch (which) {
		case 0:
			h->f2e = b->f2e8;

			if (o)
				h->f30 = o->f2e8;

			break;
		case 1:
			h->f2e = o->f2e8;

			if (o)
				h->f30 = b->f2e8;

			break;
	}

	{
		unsigned short x = GLevel, y = GGame;

		h->f36 = 0;
		h->f32 = x;
		h->f34 = y;
	}
}

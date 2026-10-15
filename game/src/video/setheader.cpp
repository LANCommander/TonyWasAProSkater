/* Video_SetHeader__FP10SVideoHeadi (0x80069394) */
/* C++ (VIDEO.CPP). */
#include "game/video.h"
#include "game/bruce.h"

extern SVideoInfo GVInfo1[2];
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

	h->NumFrames = v.NumFrames;

	for (i = 0; i < 5; i++)
		h->Highlights[i] = v.Highlights[i];

	{
		unsigned short t = GNumberOfPlayers;

		b = MechList;
		h->NumSkaters = t;
	}

	/* the other skater is linked through the item list */
	o = (CBruce *)b->mNextItem;

	switch (which) {
		case 0:
			h->Skater = b->mSkaterIndex;

			if (o)
				h->Skater2 = o->mSkaterIndex;

			break;
		case 1:
			h->Skater = o->mSkaterIndex;

			if (o)
				h->Skater2 = b->mSkaterIndex;

			break;
	}

	{
		unsigned short x = GLevel, y = GGame;

		h->Checksum = 0;
		h->Level = x;
		h->GGame = y;
	}
}

/* CollideWithSkaters__FP4CCar (0x8006bc88) */
/* Builds the object's rotation matrix and runs CollideWithSkater against the skater and its partner
   (mNextItem), unless a replay/video mode 2 is active on either player. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
}

#include "game/car.h"
#include "game/bruce.h"
#include "game/m3d.h"
#include "game/video.h"

extern SVideoInfo GVInfo1;
extern SVideoInfo GVInfo2;
extern SPSXRegion PSXRegion[];
extern CBruce *MechList;

void CollideWithSkater(CCar *, CBruce *, const MATRIX &, short *);

void CollideWithSkaters(CCar *p) {
	MATRIX m;
	SModel **info;
	CBruce *s1, *s2;
	short *x;

	if (GVInfo1.VideoMode == 2 || GVInfo2.VideoMode == 2)
		return;

	info = PSXRegion[p->mRegion].ppModels;
	s1 = MechList;
	s2 = (CBruce *)s1->mNextItem;
	/* model 4's bounding box (file data) */
	x = (short *)((char *)info[4] + 12);
	RotMatrix((SVECTOR *)&p->mTransform, &m);
	CollideWithSkater(p, s1, m, x);

	if (s2)
		CollideWithSkater(p, s2, m, x);
}

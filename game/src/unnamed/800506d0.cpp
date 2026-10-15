/* FUN_800506d0 (0x800506d0) flags=-fstructure-noalias */
/* C++ (front end): makes the preview skater idx (its front-end model, idling, lit by
   DAT_800be3f8). */
#include "game/bruce.h"
#include "game/career.h"
#include "game/m3d.h"

extern SStats Stats[];
extern SLight DAT_800be3f8;

int DAT_800d1ba8;
int SKATER_Y_POS;

void M3dMaths_SetIdentityRotation(MATRIX *m); /* identity-matrix init, one arg */

extern "C" CBruce *FUN_800506d0(int idx) {
	CBruce *p = new CBruce;

	p->mSkaterIndex = idx;
	p->InitItem(Stats[idx].FrontName);

	p->mAnimSpeed = 0x10000;
	p->CycleAnim(0, 1);

	p->mRGB = 0x404040;
	p->mPos.vx = DAT_800d1ba8;
	p->mPos.vy = SKATER_Y_POS;
	p->mPos.vz = 0x30000;

	M3dMaths_SetIdentityRotation(&p->mRotMatrix);

	if (Stats[idx].pBio[4][0] == 82 || Stats[idx].pBio[4][0] == 114)
		p->mExtraFlags |= 2;

	p->mpLight = &DAT_800be3f8;
	return p;
}

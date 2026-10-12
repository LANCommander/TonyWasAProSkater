/* FUN_8005933c (0x8005933c) flags=-fstructure-noalias */
/* C++ (credits). Creates the skater shown in the credits: new CBruce, model of the selected skater
   (or skater 0 if that slot has no name), idle anim, fixed position, identity rotation, and the
   goofy flag from the stance string ('R'/'r'). Lever: -fstructure-noalias keeps the selected index
   in a register across the mSkaterIndex store. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];
extern char DAT_800be444[];

int DAT_800d1c40;
int DAT_800d232c;
int DAT_800d2328;

extern "C" void FUN_8008b238(MATRIX *);

extern "C" CBruce *FUN_8005933c() {
	CBruce *pBruce = new CBruce;

	if (!*Stats[DAT_800d1c40].FrontName) {
		pBruce->mSkaterIndex = 0;
		pBruce->InitItem(Stats[0].FrontName);
	} else {
		pBruce->mSkaterIndex = DAT_800d1c40;
		pBruce->InitItem(Stats[DAT_800d1c40].FrontName);
	}

	pBruce->mAnimSpeed = 0x10000;

	pBruce->CycleAnim(0, 1);

	pBruce->mRGB = 0x404040;

	pBruce->mPos.Set(DAT_800d232c, DAT_800d2328, 0xe6000);
	FUN_8008b238(&pBruce->mRotMatrix);

	if (*Stats[DAT_800d1c40].pBio[4] == 'R' || *Stats[DAT_800d1c40].pBio[4] == 'r')
		pBruce->mExtraFlags |= 2;
	pBruce->mpLight = (SLight *)DAT_800be444;
	return pBruce;
}

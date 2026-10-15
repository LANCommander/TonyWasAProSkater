/* Front_CreateSkaterModel2P__Fi (0x8005622c) flags=-fstructure-noalias */
/* C++ (credits). Two-player sibling of Front_CreateSkaterModel1P: new CBruce for player Player with the
   skater index from DAT_800d2304[Player], a per-player x position, and the stance flag. Lever:
   -fstructure-noalias (the index re-reads come from the separate array expressions). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"
#include "game/m3dasm.h"

extern SStats Stats[];
extern char DAT_800be444[];

extern int DAT_800d2304[2];
extern int DAT_800d2300;
extern int DAT_800d22fc;
extern int DAT_800d22f8;

CBruce *Front_CreateSkaterModel2P(int Player) {
	CBruce *pBruce = new CBruce;

	pBruce->mSkaterIndex = DAT_800d2304[Player];

	pBruce->InitItem(Stats[DAT_800d2304[Player]].FrontName);

	pBruce->mAnimSpeed = 0x10000;

	pBruce->CycleAnim(0, 1);

	pBruce->mRGB = 0x404040;

	if (!Player)
		pBruce->mPos.Set(DAT_800d2300, DAT_800d22f8, 0x1bc000);
	else
		pBruce->mPos.Set(DAT_800d22fc, DAT_800d22f8, 0x1bc000);

	M3dMaths_SetIdentityRotation(&pBruce->mRotMatrix);

	if (*Stats[DAT_800d2304[Player]].pBio[4] == 'R' || *Stats[DAT_800d2304[Player]].pBio[4] == 'r')
		pBruce->mExtraFlags |= 2;

	pBruce->mpLight = (SLight *)DAT_800be444;
	return pBruce;
}

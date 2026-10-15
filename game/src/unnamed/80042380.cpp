/* FUN_80042380 (0x80042380) */
/* C++ (MECH.CPP): CBruce's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags)
   retail has: stops the grind/roll sounds, frees the shadow buffers, leaves MechList and destroys
   the panel's two FallingTextEffects before CSuper's destructor. */
#include "game/bruce.h"

extern "C" char DAT_800a0534[];

extern CBruce *MechList[2];
extern int NumMechs;

void SFX_Stop(uint);
void Mem_Delete(void *);
extern "C" void FUN_800427bc(CBruce *p);
extern "C" void FUN_8001dfec(FallingTextEffect *p, int a);
/* egcs-ABI calls, as retail makes them */
extern "C" void dt_6CSuper(CSuper *p, int a);
extern "C" void __dl__5CItemPv(void *p);

extern "C" void FUN_80042380(CBruce *p, int f) {
	p->__vptr = DAT_800a0534;

	if (p->mGrindSnd) {
		SFX_Stop(p->mGrindSnd);

		p->mGrindSnd = 0;
	}

	if (p->mRollSnd) {
		SFX_Stop(p->mRollSnd);

		p->mRollSnd = 0;
	}

	if (p->mpShadowSource)
		Mem_Delete(p->mpShadowSource);

	if (p->mpShadowRGB)
		Mem_Delete(p->mpShadowRGB);

	p->DeleteFrom((CBody **)MechList);

	NumMechs--;
	FUN_800427bc(p);
	FUN_8001dfec(&p->mPanel.mOutOfBoundsText, 2);
	FUN_8001dfec(&p->mPanel.mFallingTextEffect, 2);
	dt_6CSuper(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

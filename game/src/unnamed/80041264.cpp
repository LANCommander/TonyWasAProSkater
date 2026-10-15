/* FUN_80041264 (0x80041264) */
/* C++ (BADDY.CPP): CScriptOnlyBaddy's destructor (vtable slot 0), kept as the egcs-ABI function
   (this, flags) retail has; the rain droplets and smoke jet are deleted through their vtables. */
#include "game/baddy.h"
#include "game/bit.h"
#include "game/m3d.h"

extern "C" char __vt_16CScriptOnlyBaddy[];

extern CBody *BaddyList;

void SFX_Stop(uint);
void Mem_Delete(void *);
extern "C" void dt_6CBaddy(CBaddy *p, int a);
extern "C" void __dl__5CItemPv(void *p); /* egcs-ABI calls, as retail makes them */

extern "C" void FUN_80041264(CScriptOnlyBaddy *p, int f) {
	int i;

	p->__vptr = __vt_16CScriptOnlyBaddy;
	p->DeleteFrom(&BaddyList);

	if (p->mLoopingSound)
		SFX_Stop(p->mLoopingSound);

	if (p->mpSmokeJet)
		delete p->mpSmokeJet;

	if (p->mpWeather) {
		for (i = 0; i < p->mWeatherData; i++) {
			CGPolyLine *o = p->mpWeather[i].p_line;

			if (o)
				delete o;
		}

		Mem_Delete(p->mpWeather);
	}

	dt_6CBaddy(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

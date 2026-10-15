/* FUN_8003e934 (0x8003e934) */
/* CPlatform::Hit (vtable slot 5), kept as the function retail has: counts the hit; a breakable
   platform (mBaddyAttributes[1]) loses health, and dies (CBody::Die, slot 1) and shatters when it
   runs out, else flashes white. */
#include "game/platform.h"

/* an egcs vtable entry (game/data/psxdata.h PsxVtEntry) */
struct VtEntry {
	short delta, index;
	void (*fn)(char *);
};

struct Item;

int Shatter_Item(Item *p, int a, int b);

extern "C" int FUN_8003e934(CPlatform *p, int a) {
	p->mHit = p->mHit + 1;

	if (p->mBaddyAttributes[1]) {
		if (p->mHealth > 0) {
			p->mHealth = (ushort)p->mHealth - a;

			if (p->mHealth <= 0) {
				struct VtEntry *vt = (struct VtEntry *)p->__vptr;

				vt[2].fn((char *)p + vt[2].delta);
				Shatter_Item((Item *)p, 0, 1);
			} else
				p->Flash(3, 255, 255, 255);
		}
	}

	return 1;
}

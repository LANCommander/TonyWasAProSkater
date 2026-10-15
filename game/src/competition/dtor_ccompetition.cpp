/* FUN_80077130 (0x80077130) */
/* C++ for the port. The matching source is unnamed/80077130.cpp: there the destructor is the
   implicit one of a class cc1 only synthesizes as a stand-in, which a native build can't tie back
   to this address. This is the destructor of the competition object (theCompetition, see
   FUN_80077060): its five polymorphic judges are destroyed last to first through their vtables,
   then the inlined ~CClass restores CClass's vtable. */
#include "game/competition.h"

struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};

extern char DAT_800a3464[];

void __dl__6CClassPv(char *);

extern "C" void FUN_80077130(CCompetition *t, int f) {
	CJudge *first = t->mJudges, *e;
	struct VtEntry *vt;

	if (first) {
		for (e = first + 5; first != e;) {
			e--;
			vt = *(struct VtEntry **)e; /* (CClass's vptr is at +0) */
			vt[1].fn((char *)e + vt[1].delta, 2);
		}
	}

	*(char **)t = DAT_800a3464;

	if (f & 1)
		__dl__6CClassPv((char *)t);
}

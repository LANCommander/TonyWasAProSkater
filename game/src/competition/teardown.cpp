/* FUN_80077060 (0x80077060) */
/* C++ for the port. The matching source is unnamed/80077060.cpp, whose global object makes cc1 emit
   this static teardown (_GLOBAL_.D) itself; a native C++ build registers the destructor with
   atexit instead and has no function to put at this address. The body is FUN_80077130 applied to
   theCompetition, without the delete. */
#include "game/competition.h"

struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};

extern char __vt_6CClass[];

extern "C" void FUN_80077060() {
	CCompetition *t = &theCompetition;
	CJudge *first = t->mJudges, *e;
	struct VtEntry *vt;

	if (first) {
		for (e = first + 5; first != e;) {
			e--;
			vt = *(struct VtEntry **)e; /* (CClass's vptr is at +0) */
			vt[1].fn((char *)e + vt[1].delta, 2);
		}
	}

	*(char **)t = __vt_6CClass;
}

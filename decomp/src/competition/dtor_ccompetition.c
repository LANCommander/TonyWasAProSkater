/* FUN_80077130 (0x80077130) */
/* C for the port. The matching source is unnamed/80077130.cpp: there the destructor is the
   implicit one of a class cc1 only synthesizes as a stand-in, which a native build can't tie back
   to this address. This is the destructor of the competition object (theCompetition, see
   FUN_80077060): its five polymorphic elements at +40 are destroyed last to first through their
   vtables, then the inlined ~CClass restores CClass's vtable. */
struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};

extern char __vt_6CClass[];

void __dl__6CClassPv(char *);

void FUN_80077130(char *t, int f) {
	char *first = t + 40, *e;
	struct VtEntry *vt;

	if (first) {
		for (e = t + 80; first != e;) {
			e -= 8;
			vt = *(struct VtEntry **)e;
			vt[1].fn(e + vt[1].delta, 2);
		}
	}
	*(char **)t = __vt_6CClass;

	if (f & 1)
		__dl__6CClassPv(t);
}

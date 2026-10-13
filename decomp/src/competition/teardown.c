/* FUN_80077060 (0x80077060) */
/* C for the port. The matching source is unnamed/80077060.cpp, whose global object makes cc1 emit
   this static teardown (_GLOBAL_.D) itself; a native C++ build registers the destructor with
   atexit instead and has no function to put at this address. The body is FUN_80077130 applied to
   theCompetition, without the delete. */
struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};

extern char theCompetition[];
extern char __vt_6CClass[];

void FUN_80077060(void) {
	char *t = theCompetition;
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
}

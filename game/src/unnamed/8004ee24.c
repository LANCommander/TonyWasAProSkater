/* FUN_8004ee24 (0x8004ee24) */
struct VT {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	struct VT *vt;
};

struct O *TrickObList;

void FUN_8004ee24(void) {
	struct O *p = TrickObList;
	struct O *q;

	if (p) {
		for (;;) {
			p->vt->fn((char *)p + p->vt->off, 3);

			q = TrickObList;

			if (!q)
				break;

			p = q;
		}
	}
}

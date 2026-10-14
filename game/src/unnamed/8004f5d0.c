/* FUN_8004f5d0 (0x8004f5d0) */
struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	struct VT *vt;
};

struct O *RailList;

void FUN_8004f5d0(void) {
	struct O *p = RailList;
	struct O *q;

	if (p) {
		for (;;) {
			p->vt->fn((char *)p + p->vt->off, 3);

			q = RailList;

			if (!q)
				break;

			p = q;
		}
	}

	RailList = 0;
}

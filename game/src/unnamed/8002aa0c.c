/* FUN_8002aa0c (0x8002aa0c) */
struct VT {
	char pad[16];
	short off;
	short pad2;
	void (*fn)(char *);
};

struct O {
	char pad[4];
	struct O *next;
	char pad2[55];
	unsigned char c;
	char pad3[4];
	struct VT *vt;
};

void FUN_8002aa0c(struct O *p) {
	for (; p; p = p->next) {
		if (p->c == 0)
			p->vt->fn((char *)p + p->vt->off);
	}
}

/* FUN_80020c90 (0x80020c90) */
struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	char pad[68];
	struct VT *vt;
};

extern char __vt_11CShatterBit[];

void FUN_80028968(char *p, int a);
void __dl__4CBitPv(void *p);

void FUN_80020c90(char *p, int f) {
	struct O *o = *(struct O **)(p + 200);

	*(char **)(p + 68) = __vt_11CShatterBit;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);

	FUN_80028968(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

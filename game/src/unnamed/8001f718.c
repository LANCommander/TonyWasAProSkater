/* FUN_8001f718 (0x8001f718) */
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

extern char __vt_6CLight[];

void FUN_80027fa4(char *p, int a);
void __dl__4CBitPv(void *p);

void FUN_8001f718(char *p, int f) {
	struct O *o = *(struct O **)(p + 96);

	*(char **)(p + 68) = __vt_6CLight;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);

	FUN_80027fa4(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

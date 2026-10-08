/* FUN_8004f318 (0x8004f318) */
typedef struct T {
	char *vt;
	unsigned int mChecksum;
	char pad[8];
	struct T *mpNext;
} T;

T *TrickObList;

unsigned int FUN_8006ef00(char *);

int FUN_8004f318(char *o) {
	unsigned int c = FUN_8006ef00(o);
	T *p;

	for (p = TrickObList; p; p = p->mpNext)
		if (p->mChecksum == c)
			break;

	return p ? 2 : 1;
}

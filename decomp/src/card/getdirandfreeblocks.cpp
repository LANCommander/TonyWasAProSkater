/* Card_GetDirAndFreeBlocks__Fii (0x80015164) */
/* 68/69, parked. Retail (and the identical THPS2-demo twin) starts the first loop with
   `addu a0,s2,zero`: i copied from size's zero register. Ours is `move a0,$0`. S29 findings,
   checked against the egcs 1.1.2 source:
   - CSE1 folds `i = size` (and every chained-zero spelling) to const 0, since the costs tie.
   - reload_cse never swaps a CONST_INT for a register on MIPS: CONST_COSTS returns 0 for
     every CONST_INT (reload1.c:8748).
   - An empty asm forcing the copy hides i's start value from loop.c, which then adds a
     loop guard (+4 words).
   So the copy must come from an RTL shape that we haven't found. */
struct DIRENTRY {
	char name[20];
	long attr;
	long size;
	struct DIRENTRY *next;
	long head;
	char system[4];
};

extern "C" {
	extern struct DIRENTRY *firstfile(char *, struct DIRENTRY *);
	extern struct DIRENTRY *nextfile(struct DIRENTRY *);
	int sprintf(char *, const char *, ...);
}

int Utils_CompareStrings(const char *, const char *, unsigned int);

extern long FileType[];
extern char CARD_FNAME_5[];
extern char CARD_VNAME_5[];
extern struct DIRENTRY SLOT_DIR[];

int Card_GetDirAndFreeBlocks(int cnct, int slot) {
	long i;
	long size = 0;
	char key[32];
	struct DIRENTRY *d;

	for (i = 0; i < 15; i++)
		FileType[i] = 3;

	sprintf(key, "bu%1d%1d:*", cnct, slot);

	d = SLOT_DIR;
	i = 0;

	if (firstfile(key, d) == d) {
		do {
			size += d->size;

			if (Utils_CompareStrings(d->name, CARD_FNAME_5, 17))
				FileType[i] = 0;
			else if (Utils_CompareStrings(d->name, CARD_VNAME_5, 17))
				FileType[i] = 1;

			i++;
			d++;
		} while (nextfile(d) == d);
	}

	size /= 8192;
	return 15 - size;
}

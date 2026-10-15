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
#include "game/psyq.h"

struct DIRENTRY {
	char name[20];
	int attr;
	int size;
	struct DIRENTRY *next;
	int head;
	char system[4];
};

extern "C" {
	extern struct DIRENTRY *firstfile(char *, struct DIRENTRY *);
	extern struct DIRENTRY *nextfile(struct DIRENTRY *);
}

int Utils_CompareStrings(const char *, const char *, unsigned int);

extern int FileType[];
extern char CARD_FNAME[];
extern char CARD_VNAME[];
/* the directory, 40 bytes an entry (in the port PC_SaveListDir fills it with the saves' names;
   firstfile and nextfile, the PS1 card's, are stubs that find nothing) */
extern char SLOT_DIR[][40];

int Card_GetDirAndFreeBlocks(int cnct, int slot) {
	int i;
	int size = 0;
	char key[32];
	struct DIRENTRY *d;

	for (i = 0; i < 15; i++)
		FileType[i] = 3;

	sprintf(key, "bu%1d%1d:*", cnct, slot);

	d = (struct DIRENTRY *)SLOT_DIR[0];
	i = 0;

	if (firstfile(key, d) == d) {
		do {
			size += d->size;

			if (Utils_CompareStrings(d->name, CARD_FNAME + 5, 17))
				FileType[i] = 0;
			else if (Utils_CompareStrings(d->name, CARD_VNAME + 5, 17))
				FileType[i] = 1;

			i++;
			d = (struct DIRENTRY *)SLOT_DIR[i];
		} while (nextfile(d) == d);
	}

	size /= 8192;
	return 15 - size;
}

/* FUN_8007ef30 (0x8007ef30) */
int ItemCacheIndex;
int DAT_800d20e4;
int DAT_800d20e8;

struct E {
	int a;
	int b[3];
};

extern struct E ItemCache[];

void FUN_8007ef30(void) {
	int i;

	ItemCacheIndex = 0;
	DAT_800d20e4 = 0;
	DAT_800d20e8 = -1;

	for (i = 0; i < 10; i++)
		ItemCache[i].a = 0;
}

/* FUN_8006e534 (0x8006e534) */
struct E {
	char pad[64];
	char *name;
	char q[272];
};

extern struct E Stats[];
extern char DAT_800d1ea4[];
extern char s_SWITCH_800d1e9c[];

char *FUN_8006e534(char *p) {
	int r = 0;
	unsigned w = *(unsigned *)(p + 308);
	char c = *Stats[*(int *)(p + 744)].name;
	unsigned b = w >> 1;

	b &= 1;

	if (c == 'R' || c == 'r')
		r = 1;

	if (b != r)
		return s_SWITCH_800d1e9c;

	return DAT_800d1ea4;
}

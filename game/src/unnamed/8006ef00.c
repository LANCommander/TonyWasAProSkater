/* FUN_8006ef00 (0x8006ef00) */
struct T {
	char pad[12];
	int *tab;
	char q[48];
};

extern struct T PSXRegion[];

int FUN_8006ef00(char *p) {
	return PSXRegion[p[27]].tab[*(unsigned short *)(p + 22)];
}

/* FUN_8007d20c (0x8007d20c) */
extern char DAT_800a35a4[];
extern char DAT_800a35c4[];

void __dl__6CClassPv(void *p);

void FUN_8007d20c(char **p, int f) {
	p[0] = DAT_800a35a4;
	p[30] = DAT_800a35c4;
	p[17] = DAT_800a35c4;
	p[7] = DAT_800a35c4;
	p[0] = DAT_800a35c4;

	if (f & 1)
		__dl__6CClassPv(p);
}

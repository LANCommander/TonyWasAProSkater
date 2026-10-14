/* FUN_8007d900 (0x8007d900) */
extern char DAT_800a3594[];
extern char DAT_800a35c4[];

void __dl__6CClassPv(void *p);

void FUN_8007d900(char **p, int f) {
	p[0] = DAT_800a3594;
	p[2] = DAT_800a35c4;
	p[0] = DAT_800a35c4;

	if (f & 1)
		__dl__6CClassPv(p);
}

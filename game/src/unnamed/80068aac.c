/* FUN_80068aac (0x80068aac) */
extern char DAT_800a2310[];

void __dl__6CClassPv(void *p);

void FUN_80068aac(char *p, int f) {
	*(char **)(p + 0) = DAT_800a2310;

	if (f & 1)
		__dl__6CClassPv(p);
}

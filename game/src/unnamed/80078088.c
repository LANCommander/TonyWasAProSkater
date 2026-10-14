/* FUN_80078088 (0x80078088) */
extern char DAT_800a3484[];

void __dl__6CClassPv(void *p);

void FUN_80078088(char *p, int f) {
	*(char **)(p + 0) = DAT_800a3484;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_8001f2dc (0x8001f2dc) */
extern char DAT_8009efa4[];

void __dl__6CClassPv(void *p);

void FUN_8001f2dc(char *p, int f) {
	*(char **)(p + 0) = DAT_8009efa4;

	if (f & 1)
		__dl__6CClassPv(p);
}

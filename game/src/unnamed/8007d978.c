/* FUN_8007d978 (0x8007d978) */
extern char DAT_800a35c4[];

void __dl__6CClassPv(void *p);

void FUN_8007d978(char *p, int f) {
	*(char **)(p + 0) = DAT_800a35c4;

	if (f & 1)
		__dl__6CClassPv(p);
}

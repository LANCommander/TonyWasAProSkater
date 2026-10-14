/* FUN_8001664c (0x8001664c) */
extern char DAT_8009e548[];

void __dl__6CClassPv(void *p);

void FUN_8001664c(char *p, int f) {
	*(char **)(p + 0) = DAT_8009e548;

	if (f & 1)
		__dl__6CClassPv(p);
}

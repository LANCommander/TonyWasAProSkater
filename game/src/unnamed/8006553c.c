/* FUN_8006553c (0x8006553c) */
extern char DAT_800a2130[];
extern char DAT_800a21b0[];

void FUN_80065d80(char *p);
void __dl__6CClassPv(void *p);

void FUN_8006553c(char **p, int f) {
	*p = DAT_800a2130;
	FUN_80065d80((char *)p);

	*p = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

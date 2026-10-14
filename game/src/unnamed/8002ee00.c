/* FUN_8002ee00 (0x8002ee00) */
extern char __vt_11CWobblyGlow[];

int BASEF(char *p, int a);
void __dl__4CBitPv(void *p);

void FUN_8002ee00(char *p, int f) {
	*(char **)(p + 68) = __vt_11CWobblyGlow;
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* FUN_8004eba0 (0x8004eba0) */
extern char __vt_11CFlatRipple[];

int BASEF(char *p, int a);
void __dl__4CBitPv(void *p);

void FUN_8004eba0(char *p, int f) {
	*(char **)(p + 68) = __vt_11CFlatRipple;
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* FUN_8002a448 (0x8002a448) */
extern char __vt_11CSmokeTrail[];

int BASEF(char *p, int a);
void __dl__4CBitPv(void *p);

void FUN_8002a448(char *p, int f) {
	*(char **)(p + 68) = __vt_11CSmokeTrail;
	BASEF(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

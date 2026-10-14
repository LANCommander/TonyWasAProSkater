/* FUN_80043584 (0x80043584) */
extern char __vt_5CItem[];

void __dl__5CItemPv(void *p);

void FUN_80043584(char *p, int f) {
	*(char **)(p + 72) = __vt_5CItem;

	if (f & 1)
		__dl__5CItemPv(p);
}

/* _._5CItem (0x8005bb3c) */
/* The copy of CItem's destructor the credits module emitted: it restores CItem's own vtable and,
   when deleting, frees through CClass's operator delete (not CItem's __dl__5CItemPv). */
extern char __vt_5CItem[];

void __dl__6CClassPv(char *);

void __dt__5CItem(char *t, int f) {
	*(char **)t = __vt_5CItem;

	if (f & 1)
		__dl__6CClassPv(t);
}

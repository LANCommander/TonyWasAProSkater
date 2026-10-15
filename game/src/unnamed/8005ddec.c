/* FUN_8005ddec (0x8005ddec) */
/* The teardown (_GLOBAL_.D) of DAT_800bf104 (FUN_8005de08): the inlined ~CClass puts CClass's
   vtable back into its vptr. */
extern void *DAT_800bf104[];
extern char DAT_800a1b94[];

void FUN_8005ddec(void) {
	volatile int pad[4];

	DAT_800bf104[0] = DAT_800a1b94;
}

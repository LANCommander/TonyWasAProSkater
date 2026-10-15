/* FUN_8005dc58 (0x8005dc58) */
extern int DAT_800d1ce4;
extern int DAT_800d1ce8;
extern int DAT_800d1cdc;
extern int DAT_800d1ce0;
extern int DAT_800d1cd8;

extern int Vblanks;

void FUN_8005dc58(int a) {
	DAT_800d1ce4 = 1;
	DAT_800d1ce8 = 1;
	DAT_800d1cdc = 0;
	DAT_800d1ce0 = a * 60;
	DAT_800d1cd8 = Vblanks;
}

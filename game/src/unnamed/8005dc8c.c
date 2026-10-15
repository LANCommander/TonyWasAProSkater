/* FUN_8005dc8c (0x8005dc8c) */
extern int DAT_800d1ce4;
extern int DAT_800d1ce8;
extern int DAT_800d1cec;
extern int DAT_800d1d14;
extern int DAT_800d1d08;

void FUN_8005d370(void);

void FUN_8005dc8c(void) {
	DAT_800d1ce4 = 0;
	DAT_800d1ce8 = 0;
	DAT_800d1cec = 1;
	DAT_800d1d14 = 1;
	DAT_800d1d08 = 1;
	FUN_8005d370();
}

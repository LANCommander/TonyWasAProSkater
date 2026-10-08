/* FUN_80014d5c (0x80014d5c) */
int DAT_800d137c, CardLoadFlag, CardStatusMode, ev0, ev1, ev2,
    ev3, ev10, ev11, ev12, ev13;

void FUN_8008e944(void);
void FUN_8008e9d4(void);
void FUN_8008ea38(void);
void FUN_8008ea48(int);
int FUN_8008eab8(int, int, int, int);
void FUN_8008eac8(void);
void FUN_8008f0dc(void);

void FUN_80014d5c(void) {
	DAT_800d137c = 1;
	FUN_8008e944();
	FUN_8008e9d4();
	FUN_8008ea38();
	FUN_8008f0dc();

	ev0 = FUN_8008eab8(0xf4000001, 4, 0x2000, 0);
	ev1 = FUN_8008eab8(0xf4000001, 0x8000, 0x2000, 0);
	ev2 = FUN_8008eab8(0xf4000001, 0x100, 0x2000, 0);
	ev3 = FUN_8008eab8(0xf4000001, 0x2000, 0x2000, 0);
	ev10 = FUN_8008eab8(0xf0000011, 4, 0x2000, 0);
	ev11 = FUN_8008eab8(0xf0000011, 0x8000, 0x2000, 0);
	ev12 = FUN_8008eab8(0xf0000011, 0x100, 0x2000, 0);
	ev13 = FUN_8008eab8(0xf0000011, 0x2000, 0x2000, 0);
	FUN_8008eac8();
	FUN_8008ea48(ev0);
	FUN_8008ea48(ev1);
	FUN_8008ea48(ev2);
	FUN_8008ea48(ev3);
	FUN_8008ea48(ev10);
	FUN_8008ea48(ev11);
	FUN_8008ea48(ev12);
	FUN_8008ea48(ev13);

	CardStatusMode = 0;
	CardLoadFlag = 0;
}

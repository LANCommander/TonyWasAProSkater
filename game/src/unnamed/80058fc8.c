/* FUN_80058fc8 (0x80058fc8) */
int DAT_800d1c18;
int DAT_800d1c10;
int DAT_800d1c0c;
int DAT_800d2300;
int DAT_800d22fc;
int DAT_800d22f8;

void FUN_80058fc8(void) {
	DAT_800d2300 = -(DAT_800d1c0c * 112);
	DAT_800d22fc = DAT_800d1c0c * 176;
	DAT_800d22f8 = (DAT_800d1c18 - 77) * DAT_800d1c10;
}

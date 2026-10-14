/* Redbook_Rnd__Fi (0x8004794c) */
int DAT_800d2238, DAT_800d223c, DAT_800d2240;

int Redbook_Rnd(int n) {
	int r;

	DAT_800d2238 = DAT_800d2238 * DAT_800d223c + DAT_800d2240;
	r = (int)((DAT_800d2238 & 0xffff) * n);
	DAT_800d223c = (DAT_800d223c ^ DAT_800d2238) + (DAT_800d2238 >> 4);
	DAT_800d2240 -= 0x10101010;
	DAT_800d2240 += DAT_800d2238 >> 3;
	return r >> 16;
}

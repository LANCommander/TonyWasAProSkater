/* Redbook_InitialRand__Fi (0x80047928) */
int DAT_800d2238, DAT_800d223c, DAT_800d2240;

void Redbook_InitialRand(int seed) {
	DAT_800d2238 = seed;
	DAT_800d223c = 0x12b9b0a1;
	DAT_800d2240 = 0x0aa2fb3f;
}

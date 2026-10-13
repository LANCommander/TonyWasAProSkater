/* handleLidOpen__Fv (0x80023c48) */
int CdControlB(int, unsigned char *, unsigned char *);

void handleLidOpen(void) {
	unsigned char res[8];

	do
		CdControlB(1, 0, res);

	while (res[0] & 0x10);

	CdControlB(19, 0, 0);

	while (!(res[0] & 2))
		CdControlB(1, 0, res);
}

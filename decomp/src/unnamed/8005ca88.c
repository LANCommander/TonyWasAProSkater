/* FUN_8005ca88 (0x8005ca88) */
extern char *SpoolerBlocksFileIO;
extern char s_PREMgr_already_loaded[];
extern char s_Sprites_pre[];

void FUN_8008c190(char *a);
char *__nw__6CClassUi(unsigned n);
char *FUN_8006613c(char *p);
void FUN_80066214(char *p, char *a);

void FUN_8005ca88(void) {
	char *p;

	if (SpoolerBlocksFileIO)
		FUN_8008c190(s_PREMgr_already_loaded);

	p = __nw__6CClassUi(324);

	if (p)
		p = FUN_8006613c(p);

	SpoolerBlocksFileIO = p;
	FUN_80066214(p, s_Sprites_pre);
}

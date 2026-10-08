/* FUN_8005cae8 (0x8005cae8) */
struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	struct VT *vt;
};

extern struct O *SpoolerBlocksFileIO;
extern char s_No_PREMgr_to_delete[];

void FUN_8008c190(char *a);

void FUN_8005cae8(void) {
	if (SpoolerBlocksFileIO)
		FUN_8008c190(s_No_PREMgr_to_delete);

	if (SpoolerBlocksFileIO)
		SpoolerBlocksFileIO->vt->fn((char *)SpoolerBlocksFileIO + SpoolerBlocksFileIO->vt->off, 3);

	SpoolerBlocksFileIO = 0;
}

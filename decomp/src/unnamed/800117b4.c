/* FUN_800117b4 (0x800117b4) */
int CursorPos;
int SizeUp;
int GSaveAction;

extern char Pad[];
extern char SaveGameName[];
extern char s_800a42e8[];
extern char s_800a42dc[];

void FUN_8008e904(char *a, char *b);

void FUN_800117b4(void) {
	CursorPos = 0;
	SizeUp = 0;
	Pad[161] = 0;
	Pad[177] = 0;
	Pad[525] = 0;
	Pad[541] = 0;

	if (GSaveAction == 0)
		FUN_8008e904(SaveGameName, s_800a42e8);
	else
		FUN_8008e904(SaveGameName, s_800a42dc);
}

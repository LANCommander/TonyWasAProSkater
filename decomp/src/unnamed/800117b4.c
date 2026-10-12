/* FUN_800117b4 (0x800117b4) */
#include "game/padbuttons.h"
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
	Pad[PAD_MENU_UP + PAD_TRIGGERED] = 0;
	Pad[PAD_MENU_DOWN + PAD_TRIGGERED] = 0;
	Pad[PAD_SIZE + PAD_MENU_UP + PAD_TRIGGERED] = 0;
	Pad[PAD_SIZE + PAD_MENU_DOWN + PAD_TRIGGERED] = 0;

	if (GSaveAction == 0)
		FUN_8008e904(SaveGameName, s_800a42e8);
	else
		FUN_8008e904(SaveGameName, s_800a42dc);
}

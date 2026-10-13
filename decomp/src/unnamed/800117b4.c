/* InitEnterName__Fv (0x800117b4) */
#include "names.h"
#include "game/padbuttons.h"

int CursorPos;
int SizeUp;
int GSaveAction;

extern char Pad[];
extern char SaveGameName[];
extern char s_800a42e8[];
extern char s_800a42dc[];

void strcpy(char *a, char *b);

void InitEnterName(void) {
	CursorPos = 0;
	SizeUp = 0;
	Pad[PAD_MENU_UP + PAD_TRIGGERED] = 0;
	Pad[PAD_MENU_DOWN + PAD_TRIGGERED] = 0;
	Pad[PAD_SIZE + PAD_MENU_UP + PAD_TRIGGERED] = 0;
	Pad[PAD_SIZE + PAD_MENU_DOWN + PAD_TRIGGERED] = 0;

	if (GSaveAction == 0)
		strcpy(SaveGameName, s_800a42e8);
	else
		strcpy(SaveGameName, s_800a42dc);
}

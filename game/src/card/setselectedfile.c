/* Card_SetSelectedFile__FPc (0x80014218) */
#include "names.h"

extern char CARD_FNAME[];
extern char CARD_VNAME[];

#include "pc_saves.h"

void Card_SetSelectedFile(char *p) {
	/* PC: p is a SLOT_DIR entry filled by PC_SaveListDir: the save's name */
	PC_SaveSelect(p);
	return;
	CARD_FNAME[22] = p[17];
	CARD_FNAME[23] = p[18];
	CARD_VNAME[22] = p[17];
	CARD_VNAME[23] = p[18];
}

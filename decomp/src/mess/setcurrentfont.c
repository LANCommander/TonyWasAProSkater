/* Mess_SetCurrentFont__FPc (0x8001f144) */
typedef struct {
	char name[0x1a];
	unsigned short scale;
	char pad2[8];
} Font;

int Num_fonts;
int Current_font;

extern Font Font_table[];

extern int strcmp(char *, char *);

void Mess_SetCurrentFont(char *name) {
	int i;

	for (i = 0; i < Num_fonts; i++) {
		if (!strcmp(name, Font_table[i].name))
			break;
	}

	Current_font = i;
}

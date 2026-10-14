/* Mess_GetFontScale__Fv (0x8001f22c) */
typedef struct {
	char pad[0x1a];
	unsigned short scale;
	char pad2[8];
} Font;

unsigned char NewFont;
int Current_font;

extern Font Font_table[];

int Mess_GetFontScale(void) {
	if (!NewFont)
		return 0;

	return (Current_font + Font_table)->scale;
}

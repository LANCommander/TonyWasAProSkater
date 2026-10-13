/* Mess_UnloadFont__Fv (0x8001ef60) */
#include "names.h"

typedef struct {
	char pad[16];
	int count;
	char *ptr;
	char rest[12];
} Font;

extern Font Font_table[];

int Current_font, Num_fonts;

void Pack_Unpack(short, short);
void Free16Slot(unsigned short);
void Mem_Delete(char *);

void Mess_UnloadFont(void) {
	int i;

	for (i = 0; i < Font_table[Current_font].count; i++) {
		char *p = (char *)(i * 36 + (int)Font_table[Current_font].ptr);

		Pack_Unpack(*(short *)(p + 8), *(short *)(p + 12));
	}

	Free16Slot(*(unsigned short *)(Font_table[Current_font].ptr + 22));
	Mem_Delete(Font_table[Current_font].ptr);

	Font_table[Current_font].ptr = 0;

	{
		int n = Num_fonts, i, m;

		if (Current_font == n - 1) {
			for (;;) {
				if (n <= 0)
					return;

				m = n - 1;
				i = n;
				Num_fonts = m;
				n = m;

				if (Font_table[i - 2].ptr)
					return;
			}
		}
	}
}

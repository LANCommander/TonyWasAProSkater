/* Front_SetSJISName__FPc (0x800116b4) */
/* C++ (RFRONT.CPP). Demo twin; locals from its MAIN.SYM. Converts the 10-character high-score name
   entry buffer to Shift-JIS for the memory card title: trailing blanks are trimmed, embedded holes
   become spaces, A-Z map to full-width letters (0x82, c+31) and spaces to 0x81 0x40. */
extern char SaveGameName[];

void Front_SetSJISName(char *SJISName) {
	int i;

	for (i = 9; i > 0; i--) {
		if (SaveGameName[i] != ' ' && SaveGameName[i])
			break;

		SaveGameName[i] = 0;
	}

	for (; i >= 0; i--) {
		if (!SaveGameName[i])
			SaveGameName[i] = ' ';
	}

	for (i = 0; i < 10; i++) {
		if (!SaveGameName[i]) {
			*SJISName = 0;
			return;
		}

		if ((unsigned char)(SaveGameName[i] - 'A') < 26) {
			*SJISName++ = 130;
			*SJISName++ = SaveGameName[i] + 31;
		} else if (SaveGameName[i] == ' ') {
			*SJISName++ = 129;
			*SJISName++ = 64;
		}
	}
}

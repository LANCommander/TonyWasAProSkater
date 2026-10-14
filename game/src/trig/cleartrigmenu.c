/* Trig_ClearTrigMenu__Fv (0x800447f4) */
extern int MenuFilenamePointers[];

int NumTrigMenuEntries;

void Trig_ClearTrigMenu(void) {
	int i;

	for (i = 0; i < 40; i++)
		MenuFilenamePointers[i] = 0;

	NumTrigMenuEntries = 0;
}

/* Trig_ClearTrigMenu__Fv (0x800447f4) */
extern char *MenuFilenamePointers[40];
extern int NumTrigMenuEntries;

void Trig_ClearTrigMenu() {
	int i;

	for (i = 0; i < 40; i++)
		MenuFilenamePointers[i] = 0;

	NumTrigMenuEntries = 0;
}

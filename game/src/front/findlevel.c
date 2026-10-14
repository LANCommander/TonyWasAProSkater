/* Front_FindLevel__FPc (0x8001cb30) */
typedef struct {
	char pad[20];
	char *name;
	char pad2[96];
} SLevel;

extern SLevel Levels[];
extern int CurrentGameLevel;

extern int Utils_CompareStrings(const char *, const char *, unsigned);

SLevel *Front_FindLevel(char *s) {
	SLevel *r = 0;
	int i = 0;

	while (*Levels[i].name) {
		if (Utils_CompareStrings(Levels[i].name, s, 0xffffffff)) {
			CurrentGameLevel = i;
			r = &Levels[i];
			break;
		}

		if (++i >= 41)
			break;
	}

	return r;
}

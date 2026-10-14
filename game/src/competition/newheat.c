/* Competition_NewHeat__Fv (0x800764a4) */
typedef struct {
	int pad[4];
	int a, b, c, pad2, d, e;
	char pad3[0xc0 - 0x28];
	unsigned char heat;
} SComp;

extern SComp theCompetition;

int Competition_NewHeat(void) {
	theCompetition.b = 0;
	theCompetition.a = 0;
	theCompetition.c = 0;
	theCompetition.d = 0;
	theCompetition.e = 0;
	return ++theCompetition.heat < 3;
}

/* Horse_Reset__Fv (0x800162bc) */
extern int GHorseSounded;

extern int GHorsePlayer, GHorseMode, GHorseScore, GHorseDoneStart, GHorseRestart, GHorseLetters[2],
    GHorseCursorPos, GHorseWordChosen, GHorseClearInput2;

void Horse_Reset(void) {
	GHorseSounded = 0;
	GHorsePlayer = 0;
	GHorseMode = 0;
	GHorseScore = 0;
	GHorseDoneStart = 0;
	GHorseRestart = 0;
	GHorseLetters[0] = 0;
	GHorseLetters[1] = 0;
	GHorseCursorPos = 0;
	GHorseWordChosen = 0;
	GHorseClearInput2 = 1;
}

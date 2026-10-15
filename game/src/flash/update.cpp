/* Flash_Update__Fv (0x8001f3f4) */
extern int Fading;
extern int FadeCountdown;
extern int FlashCountdown;
extern char CurrentImportance;
extern int CurrentR;
extern int CurrentG;
extern int CurrentB;
extern int dR;
extern int dG;
extern int dB;

void Flash_Update(void) {
	if (Fading) {
		if (FadeCountdown) {
			FadeCountdown--;

			if (FadeCountdown == 0) {
				CurrentR = CurrentG = CurrentB = 0xff0000;
			} else {
				CurrentR += dR;
				CurrentG += dR;
				CurrentB += dR;
			}
		}
	} else if (FlashCountdown) {
		FlashCountdown--;

		if (FlashCountdown == 0) {
			CurrentImportance = 0;
		} else {
			CurrentR -= dR;
			CurrentG -= dG;
			CurrentB -= dB;
		}
	}
}

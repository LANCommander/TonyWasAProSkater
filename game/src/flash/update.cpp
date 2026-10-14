/* Flash_Update__Fv (0x8001f3f4) */
int Fading;
int FadeCountdown;
int FlashCountdown;
char CurrentImportance;
int CurrentR;
int CurrentG;
int CurrentB;
int dR;
int dG;
int dB;

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

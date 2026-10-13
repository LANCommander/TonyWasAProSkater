/* FUN_8005c780 (0x8005c780) */
#include "names.h"

extern int SFXLEVEL;

int DAT_800d1cc0;
int DAT_800d1cc4;
short DAT_800d1ca0;
int DAT_800d1ca4;
int DAT_800d1cbc;

void SFX_ModifyVol(int a, int b, int c);
void SFX_Stop(int a);
int SFX_PlayX(int a, int b, int c);

void FUN_8005c780(void) {
	int s = SFXLEVEL;

	SFXLEVEL = DAT_800d1cc0;

	if (DAT_800d1cc4) {
		SFX_ModifyVol(DAT_800d1ca4, DAT_800d1ca0, DAT_800d1ca0);

		if (DAT_800d1ca0 >= 0) {
			DAT_800d1ca0 -= 256;
		} else {
			SFX_Stop(DAT_800d1ca4);

			DAT_800d1cc4 = 0;

			if (DAT_800d1cbc != -1)
				DAT_800d1ca4 = SFX_PlayX(DAT_800d1cbc, DAT_800d1ca0, 0);

			DAT_800d1ca0 = 0;
		}
	} else if (DAT_800d1cbc != -1) {
		SFX_ModifyVol(DAT_800d1ca4, DAT_800d1ca0, DAT_800d1ca0);

		if (DAT_800d1ca0 < 16384)
			DAT_800d1ca0 += 256;
		else
			DAT_800d1ca0 = 16383;
	}

	SFXLEVEL = s;
}

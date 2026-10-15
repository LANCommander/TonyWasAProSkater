/* SyncToGoodState__Fv (0x80014424) */
#include "names.h"
#include "pc_frame.h"

extern int CardStatusMode, CardLoadFlag, CardStatusTemp, CardStatus;

void VSync(int);
int Card_CheckStatus(int, int);

int SyncToGoodState(void) {
	int n = 200;

	/* PC: no memory card to wait for (see Card_CheckStatus) */
	CardStatusMode = 0;
	CardLoadFlag = 1;
	CardStatusTemp = 1;
	CardStatus = 1;
	return 0;

	CardStatusMode = 0;
	CardLoadFlag = 0;

	for (;;) {
		PC_FrameWait(1);
		Card_CheckStatus(0, 0);

		if (CardLoadFlag == 1)
			break;

		if (--n < 0) {
			CardStatusTemp = 0;
			CardStatus = 0;
			return 1;
		}
	}

	CardStatusTemp = 1;
	CardStatus = 1;
	return 0;
}

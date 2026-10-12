/* Card_CheckStatus__Fii (0x80015278) */
#include "names.h"
int CardStatusMode, CardLoadFlag, CardStatusTemp, CardStatus, EVENT_TIME_OUT;

extern int GHasCareerLoaded;

int FUN_8008e914(int);
int _card_event(void);
void _clear_event(void);
int FUN_8008f0cc(int);
void _clear_event_x(void);
void FUN_8008ea68(int);
void _card_event_x(void);

int Card_CheckStatus(int a, int b) {
	int s0 = a * 16 + b;
	int ev;

#ifdef PC_PORT
	/* PC: no memory card, the saves are files (pc_saves.h); always ready. */
	CardStatusMode = 0;
	CardLoadFlag = 1;
	CardStatusTemp = 1;
	CardStatus = 1;
	return CardStatus;
#endif

	switch (CardStatusMode) {
		case 0:
			FUN_8008e914(s0);

			CardStatusMode = 1;
			EVENT_TIME_OUT = 0;
			CardStatus = 0;
			break;
		case 1:
			ev = _card_event();

			if (ev == 0)
				break;

			switch (ev) {
				case 1:
					CardStatusTemp = 1;

					if (CardLoadFlag == 1)
						CardStatusMode = 4;
					else
						CardStatusMode = 2;

					break;
				case 2:
					CardStatusTemp = -3;
					CardStatusMode = 4;
					CardLoadFlag = 0;
					break;
				case 3:
					CardStatusTemp = -1;
					CardStatusMode = 4;
					CardLoadFlag = 0;
					break;
				case 4:
					GHasCareerLoaded = 0;
					CardStatusTemp = 2;
					_clear_event_x();
					FUN_8008ea68(s0);
					_card_event_x();

					CardStatusMode = 2;
					CardLoadFlag = 0;
					break;
				default:
					CardStatusTemp = -3;
					CardStatusMode = 4;
					CardLoadFlag = 0;
					break;
			}

			break;
		case 2:
			_clear_event();
			FUN_8008f0cc(s0);

			CardStatusMode = 3;
			EVENT_TIME_OUT = 0;
			break;
		case 3:
			ev = _card_event();

			if (ev == 0)
				break;

			CardStatusMode = 4;

			switch (ev) {
				case 1:
					CardLoadFlag = 1;
					break;
				case 2:
					CardStatusTemp = -3;
					CardLoadFlag = 0;
					break;
				case 3:
					CardStatusTemp = -1;
					CardLoadFlag = 0;
					break;
				case 4:
					CardStatusTemp = -2;
					CardLoadFlag = 0;
					break;
				default:
					CardStatusTemp = -3;
					CardLoadFlag = 0;
					break;
			}

			break;
		case 4:
			CardStatus = CardStatusTemp;
			CardStatusMode = 0;
			break;
	}

	return CardStatus;
}

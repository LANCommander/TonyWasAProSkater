/* _card_event__Fv (0x80014ee8) */
#include "names.h"
int ev0, ev1, ev2, ev3, EVENT_TIME_OUT;

int FUN_8008eb08(int);

int _card_event(void) {
	int r = FUN_8008eb08(ev0) == 1;

	if (FUN_8008eb08(ev1) == 1)
		r = 2;

	if (FUN_8008eb08(ev2) == 1)
		r = 3;

	if (FUN_8008eb08(ev3) == 1)
		r = 4;

	if (EVENT_TIME_OUT++ >= 120)
		r = 2;

	return r;
}

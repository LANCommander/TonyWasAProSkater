/* _card_event_x__Fv (0x80014fd4) */
#include "names.h"

extern int ev10, ev11, ev12, ev13;

int FUN_8008eb08(int);

int _card_event_x(void) {
	for (;;) {
		if (FUN_8008eb08(ev10) == 1)
			return 1;

		if (FUN_8008eb08(ev11) == 1)
			return 2;

		if (FUN_8008eb08(ev12) == 1)
			return 3;

		if (FUN_8008eb08(ev13) == 1)
			return 4;
	}
}

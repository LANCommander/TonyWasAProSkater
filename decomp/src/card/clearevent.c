/* _clear_event__Fv (0x80014f8c) */
#include "names.h"
int ev0, ev1, ev2, ev3;

int FUN_8008eb08(int);

void _clear_event(void) {
	FUN_8008eb08(ev0);
	FUN_8008eb08(ev1);
	FUN_8008eb08(ev2);
	FUN_8008eb08(ev3);
}

/* _clear_event_x__Fv (0x80015044) */
#include "names.h"

int ev10;
int ev11;
int ev12;
int ev13;

void FUN_8008eb08(int a);

void _clear_event_x(void) {
	FUN_8008eb08(ev10);
	FUN_8008eb08(ev11);
	FUN_8008eb08(ev12);
	FUN_8008eb08(ev13);
}

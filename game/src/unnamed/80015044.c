/* _clear_event_x__Fv (0x80015044) */
#include "names.h"

extern int ev10;
extern int ev11;
extern int ev12;
extern int ev13;

void FUN_8008eb08(int a);

void _clear_event_x(void) {
	FUN_8008eb08(ev10);
	FUN_8008eb08(ev11);
	FUN_8008eb08(ev12);
	FUN_8008eb08(ev13);
}

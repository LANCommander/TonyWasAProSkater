/* FUN_800491a0 (0x800491a0) */
#include "names.h"

void Db_CallBack(void);
void FUN_8008c370(void (*f)(void));

void FUN_800491a0(void) {
	FUN_8008c370(Db_CallBack);
}

/* FUN_800491a0 (0x800491a0) */
#include "names.h"
#ifdef PC_PORT
#include "pc_draw.h"
#endif

void Db_CallBack(void);
void FUN_8008c370(void (*f)(void));

void FUN_800491a0(void) {
#ifdef PC_PORT
	PC_DrawSetDone((void (*)(void))(Db_CallBack));
#else
	FUN_8008c370(Db_CallBack);
#endif
}

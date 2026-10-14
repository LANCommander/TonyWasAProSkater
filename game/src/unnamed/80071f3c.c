/* Init_ForGame__Fv (0x80071f3c) */
#include "names.h"

void Db_Init(void);
void Front_Init(void);
void Video_InitOnce(void);
void FUN_8004e1d4(void);
void FUN_8006952c(void);
void FUN_80082054(void);

void Init_ForGame(void) {
	Db_Init();
	Front_Init();
	Video_InitOnce();
	FUN_8004e1d4();
	FUN_8006952c();
	FUN_80082054();
}

/* FUN_80023528 (0x80023528) */
#include "names.h"

short GameFMV_StartX;

struct S {
	int a, b, c;
};

void GameFMV_StopFMV(void);
void FUN_80023168(int a);
void FUN_80022aec(struct S *p);
void GameFMV_StartFMV(int a);

void FUN_80023528(void) {
	struct S st;
	struct S *s;

	GameFMV_StopFMV();

	s = &st;
	st.a = 0;
	s->b = 0;
	s->c = 0;
	FUN_80023168(2);
	FUN_80022aec(s);
	GameFMV_StartFMV(GameFMV_StartX != -1);
}

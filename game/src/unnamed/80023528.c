/* GameFMV_SkipTrack__Fv (0x80023528) */
#include "names.h"

extern short GameFMV_StartX;

struct S {
	int a, b, c;
};

void GameFMV_StopFMV(void);
void GameFMV_SetStartTrack(int a);
void GameFMV_AddSoundSource(struct S *p);
void GameFMV_StartFMV(int a);

void GameFMV_SkipTrack(void) {
	struct S st;
	struct S *s;

	GameFMV_StopFMV();

	s = &st;
	st.a = 0;
	s->b = 0;
	s->c = 0;
	GameFMV_SetStartTrack(2);
	GameFMV_AddSoundSource(s);
	GameFMV_StartFMV(GameFMV_StartX != -1);
}

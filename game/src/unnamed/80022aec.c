/* GameFMV_AddSoundSource__FPC7CVector (0x80022aec) */
#include "names.h"

struct V {
	int x, y, z;
};

extern int GameFMV_Active;
extern int GameFMV_NumSoundSources;

extern struct V DAT_800a6388[];

void GameFMV_AddSoundSource(struct V *s) {
	int n;

	if (GameFMV_Active == 0) {
		n = GameFMV_NumSoundSources;

		if (n < 8) {
			DAT_800a6388[n] = *s;
			GameFMV_NumSoundSources = n + 1;
		}
	}
}

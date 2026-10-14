/* FUN_8004ba34 (0x8004ba34) */
#include "pc_sound.h"
int nko_flag;
int keyon_queue;
int keyoff_queue;
int voiceon_flag;
int exp_flag;
int loop_flag;
int voicefree_flag;
char reverb_changed;

void FUN_800947e4(int a, int b);

void FUN_8004ba34(void) {
	PC_SoundKeyOff(0xffffffff);

	nko_flag = 0;
	keyon_queue = 0;
	keyoff_queue = 0;
	voiceon_flag = 0;
	exp_flag = 0;
	loop_flag = 0;
	voicefree_flag = 0xffffff;
	reverb_changed = 0;
}

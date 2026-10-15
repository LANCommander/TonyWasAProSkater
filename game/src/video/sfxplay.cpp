/* Video_SFXPlay__Fisi (0x80068e8c) */
/* C++ (VIDEO.CPP). */
#include "game/video.h"

enum ESoundCmdType { SOUNDCMD_NONE, SOUNDCMD_PLAY, SOUNDCMD_STOP };

extern SVideoInfo GVInfo1[2];
extern int GamePaused;

void QueueSoundCmd(ESoundCmdType, int);

void Video_SFXPlay(int a, short b, int c) {
	if (GVInfo1[0].VideoMode == VIDEO_RECORD && GamePaused == 0)
		QueueSoundCmd(SOUNDCMD_PLAY, a);
}

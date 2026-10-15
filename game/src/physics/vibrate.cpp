/* Vibrate__6CBruceiii (0x8002f8f0) */
/* C++ (PHYSICS.CPP). Runs the pad's actuator, unless vibration is off for this player (horse swaps
   pads) or a replay video is playing. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"
#include "game/video.h"

struct Message;

extern int GGame;
extern int GHorsePlayer;
extern SCareer GCareer;
extern SVideoInfo GVInfo1[2];

void Pad_ActuatorOn(uchar, ushort, uchar, uchar);

void CBruce::Vibrate(int a, int b, int c) {
	int Pad = mPadIndex;

	if (GGame == 7)
		Pad ^= GHorsePlayer;

	if (GCareer.Options.VibrationEnabled[Pad] == 0)
		return;

	if (GVInfo1[0].VideoMode == 2)
		return;

	if (GVInfo1[1].VideoMode == 2)
		return;

	Pad_ActuatorOn(Pad, a, b, c);
}

/* SendDeathPulse__6CBaddy (0x8003fb68) */
/* C++ (BADDY.CPP). Once only, and only for a baddy placed at a node: pulses the node's links. */
#include "game/baddy.h"

extern char **OffsetList;

void Trig_SendPulse(ushort *);

void CBaddy::SendDeathPulse() {
	if (!mDeathPulseSent && mNode != 0xffff) {
		mDeathPulseSent = 1;
		Trig_SendPulse((ushort *)(OffsetList[mNode] + 6));
	}
}

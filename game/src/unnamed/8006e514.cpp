/* Set__6CTrickP7MessagePcii (0x8006e514) */
/* C++ (PANEL.CPP). Fills in a trick queue entry. */
#include "game/panel.h"

void CTrick::Set(Message *pMessage, char *pName, int Points, int Depreciate) {
	message = pMessage;
	name = pName;
	point_value = Points;
	countdown = 18;
	depreciate = Depreciate;
}

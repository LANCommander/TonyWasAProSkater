/* TotalScore__10CPanelData (0x8006ebdc) */
/* C++ (PANEL.CPP). Sums the point values of the tricks in the queue. */
#include "game/panel.h"

int CPanelData::TotalScore() {
	int Total = 0;

	for (int i = 0; i < mTQStack; i++)
		Total += mTQ[i].point_value;

	return Total;
}

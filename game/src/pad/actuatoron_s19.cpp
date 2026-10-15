/* Pad_ActuatorOn__FUcUsUcUc (0x80070a80) */
#include "game/pad.h"

extern SControl Pad[];
extern int GGame;

uchar align[8];

#include "pc_input.h"

extern "C" {
	int PadGetState(int);
	void PadSetAct(int, uchar *, int);
	int PadSetActAlign(int, uchar *);
}

void Pad_ActuatorOn(uchar index, ushort time, uchar motor, uchar motor_value) {
	int checkIndex = 0;
	int state;

	if (GGame != 7)
		checkIndex = index;

	/* PC: Pad_Update hands the motor values to the player's gamepad (pc_input.h) */
	state = PC_InputGamepadConnected(index) ? 2 : 0;
	if (state != 0) {
		Pad[checkIndex].AlignCalled = 1;

		{
			uchar *p_motor_value = &(&Pad[index].Motor0)[motor];
			ushort *p_motor_timer;
			ushort *q;

			*p_motor_value = motor_value;
			q = &Pad[index].Motor0Timer;
			p_motor_timer = (ushort *)((char *)q + (motor << 1));
			*p_motor_timer = time;
		}
	}
}

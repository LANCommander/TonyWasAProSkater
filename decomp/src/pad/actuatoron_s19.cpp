/* Pad_ActuatorOn__FUcUsUcUc (0x80070a80) */
#include "game/pad.h"

extern SControl Pad[];
extern int GGame;

uchar align[8];

#ifdef PC_PORT
#include "pc_input.h"
#endif

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

#ifdef PC_PORT
	/* PC: Pad_Update hands the motor values to the player's gamepad (pc_input.h) */
	state = PC_InputGamepadConnected(index) ? 2 : 0;
	if (state != 0) {
		Pad[checkIndex].AlignCalled = 1;
#else
	state = PadGetState(index ? 16 : 0);

	if (state != 0) {
		if (state == 1)
			Pad[checkIndex].AlignCalled = 0;

		if (!Pad[checkIndex].AlignCalled) {
			PadSetAct(index ? 16 : 0, &Pad[index].Motor0, 2);

			if (state == 2)
				Pad[checkIndex].AlignCalled = 1;
			else if (state == 6)
				if (PadSetActAlign(index ? 16 : 0, align))
					Pad[checkIndex].AlignCalled = 1;
		}
#endif

		{
			uchar *p_motor_value = &(&Pad[index].Motor0)[motor];
			ushort *p_motor_timer;
			ushort *q;

			*p_motor_value = motor_value;
			q = &Pad[index].Motor0Timer;
			p_motor_timer = (ushort *)((motor << 1) + (int)q);
			*p_motor_timer = time;
		}
	}
}

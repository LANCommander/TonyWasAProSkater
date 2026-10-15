/* Pad_ActuatorOff__FUcUc (0x80070c24) */
#include "game/padbuttons.h"
extern char Pad_154[];

void Pad_ActuatorOff(unsigned char a, unsigned char b) {
	int o = a * PAD_SIZE;
	char *p = Pad_154;
	char *q = p + 4;

	*(p + o + b) = 0;

	{
		short *s = (short *)(q + o);
		short *t = s + b;

		*t = 0;
	}
}

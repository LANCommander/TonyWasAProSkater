/* FUN_8005c68c (0x8005c68c) */
/* Hold START+SELECT on either pad to count DAT_800d2348 down from 110 (eating the START trigger);
   at zero stop the looping sound and set DAT_800d1cb8. Called from 9 sites. */
typedef unsigned char uchar;

struct SButton {
	uchar Pressed;
	uchar Triggered;
	int PressedTime;
	int ReleasedTime;
	int TriggeredTime;
};

struct SControl {
	SButton Triangle, Square, Circle, X;
	SButton LeftOne, LeftTwo, RightOne, RightTwo;
	SButton Left, Right, Up, Down;
	SButton AnalogueLeft, AnalogueRight, Start, Select;
	SButton Crouch, Jump, SmartBomb, SelectWeapon;
	int rest[11];
};

extern SControl Pad[2]; /* Pad */

int DAT_800d2348;      /* quit countdown */
unsigned DAT_800d1ca4; /* looping sound */
short DAT_800d1ca0;
int DAT_800d1cb8;

void SFX_Stop(unsigned);

void FUN_8005c68c(void) {
	if (!DAT_800d2348) {
		SFX_Stop(DAT_800d1ca4);

		DAT_800d1ca0 = 0;
		DAT_800d1cb8 = 1;
	} else if (Pad[0].Start.Pressed && Pad[0].Select.Pressed) {
		DAT_800d2348--;
		Pad[0].Start.Triggered = 0;
	} else if (Pad[1].Start.Pressed && Pad[1].Select.Pressed) {
		DAT_800d2348--;
		Pad[1].Start.Triggered = 0;
	} else {
		DAT_800d2348 = 110;
	}
}

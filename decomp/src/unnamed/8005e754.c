/* FUN_8005e754 (0x8005e754) */
typedef struct {
	int a, b, c, d, e, f;
} S;

extern unsigned char Pad[];
extern S DAT_800a1c44;

int DAT_800d1d2c;

int FUN_80070ff4(void);
int FUN_8005e0e0(S *, S *, int, int, int *, void *, int, int, int);
int FUN_8005e8e0(int);
int FUN_8005ea00(int);
int FUN_80060274(int);
int FUN_800604c8(int);
int FUN_80060854(int);
void FUN_8005e728(int);

int FUN_8005e754(int a) {
	S s;
	S z;
	int r;

	FUN_80070ff4();

	if (Pad[0x40] && Pad[0x60])
		DAT_800d1d2c = 1;

	for (;;) {
		s = DAT_800a1c44;
		z.a = 0;
		z.b = 0;
		z.c = 0;
		z.d = 0;
		z.e = 0;
		z.f = 0;
		r = FUN_8005e0e0(&s, &z, 6, 0, &a, FUN_8005e728, 0, 0, 0);

		if (r != -1) {
			switch (r) {
				case 1:
					r = FUN_8005e8e0(a);
					break;
				case 2:
					r = FUN_8005ea00(a);
					break;
				case 6:
					r = FUN_800604c8(a);
					break;
				case 7:
					r = FUN_80060854(a);
					break;
				case 5:
					r = FUN_80060274(a);
					break;
			}

			if (r == -1) {
				a = 0;
				continue;
			}
		}

		break;
	}

	return r;
}

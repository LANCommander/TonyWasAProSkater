/* FUN_8007d6e4 (0x8007d6e4) */
typedef struct {
	void *a; /* vptr */
	int b, c, d, e;
} Q;

typedef struct {
	int x, y, z, pad;
} V;

extern V DemoFocus_8, DAT_800d3a04, DAT_800d3a14, DAT_800d3a24, DAT_800d3a34, DAT_800d3a44,
    DAT_800d3a54, DAT_800d3a64, DAT_800d3a74, DAT_800d3a84, DAT_800d3a94, FlyFocus, DAT_800d3ab4;
extern Q UpDownPosition_8, testquat2, testquat3;
extern char DAT_800a35b4[];

typedef struct {
	short x, y, z;
} SV;

typedef struct {
	char x, y, z;
} CV;

extern SV LandShakeSpeed, SmallShakeAmp, MediumShakeAmp, BigShakeAmp;
extern CV LandShakeDecay;

static inline void VI(V *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

static inline void QI(Q *p, char *a, int b, int c, int d, int e) {
	p->a = a;
	p->b = b;
	p->c = c;
	p->d = d;
	p->e = e;
}

static inline void SVI(SV *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

static inline void CVI(CV *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

void FUN_8007d6e4(void) {
	VI(&DAT_800d3ab4, 0, (int)0xffe70000, (int)0x0);
	VI(&FlyFocus, 0, (int)0xfff9c000, (int)0x0);
	VI(&DAT_800d3a94, 0, (int)0xfff60000, (int)0xffc4a000);
	VI(&DAT_800d3a84, 0, (int)0xfff9c000, (int)0x0);
	VI(&DAT_800d3a74, 0, (int)0xfff6a000, (int)0x28a000);
	VI(&DAT_800d3a64, 0, (int)0xfff9c000, (int)0x0);
	VI(&DAT_800d3a54, 0, (int)0xffbb4000, (int)0x514000);
	VI(&DAT_800d3a44, 0, (int)0xfff9c000, (int)0xffce0000);
	VI(&DAT_800d3a34, 0, (int)0xff736000, (int)0x190000);
	VI(&DAT_800d3a24, 0, (int)0xfff9c000, (int)0xffed4000);
	VI(&DAT_800d3a14, 0, (int)0x64000, (int)0xf92a0000);
	VI(&DAT_800d3a04, 0, (int)0xfff9c000, (int)0x0);
	SVI(&BigShakeAmp, 25, 0, 50);
	SVI(&MediumShakeAmp, 0, 0, 30);
	SVI(&SmallShakeAmp, 0, 0, 25);
	CVI(&LandShakeDecay, 3, 0, 3);
	SVI(&LandShakeSpeed, 600, 0, 600);
	VI(&DemoFocus_8, 0, (int)0xfff92000, 0);
	QI(&UpDownPosition_8, DAT_800a35b4, 0, 0, 0, 0x1000);
	QI(&testquat2, DAT_800a35b4, 0, 0, 0, 0x1000);
	QI(&testquat3, DAT_800a35b4, 0, 0, 0, 0x1000);
}

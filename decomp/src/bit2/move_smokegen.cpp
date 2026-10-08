/* Move__15CSmokeGenerator (0x80027450) */
/* C++ (BIT2.CPP). Demo twin; locals from its MAIN.SYM. Spawns mCount motion-blur smoke bits rising
   at a random speed, then ages the generator and kills it when its life runs out. The CVector
   inline constructor and the CBit/CFlatBit inline setters (int params) give retail's prologue and
   the hoisted `100` in s4. */
struct CVector {
	int x, y, z;
	CVector() {}
	CVector(int _x, int _y, int _z) {
		x = _x;
		y = _y;
		z = _z;
	}
};

struct CBit {
	char _0[0x40];
	short m40;
	char _42[0x60 - 0x42];
	void SetM40(int v) {
		m40 = v;
	}
};

struct CFlatBit : CBit {
	short m60;
	void SetM60(int v) {
		m60 = v;
	}
};

struct CMotionBlur : CFlatBit {};

int Rnd(int n);

extern "C" {
	CMotionBlur *__nw__4CBitUi(int n); /* CBit::operator new */
	CMotionBlur *__11CMotionBlurRC7CVectorR7CVectorPciii(CMotionBlur *, char *, CVector *, char *, int, int, int);
	void SetTint__7CFT4BitUcUcUc(CMotionBlur *, int, int, int);
	void Die__4CBit(void *);

	extern char s_Smoke[];
}

struct CSmokeGenerator {
	char _0[8];
	unsigned short mLife;
	unsigned short mMaxLife;
	char _c[0xc];
	char mPos[0x30];
	int mCount;
	unsigned char mR, mG, mB, _4f;
	int mSpeed;
	int mSpeedRnd;
	int mBlurLife;
	int mBlurLifeRnd;
	void Move();
};

void CSmokeGenerator::Move() {
	CVector upvel(0, 0, 0);
	int i;

	for (i = 0; i < mCount; i++) {
		CMotionBlur *foo;

		upvel.y = -(mSpeed + Rnd(mSpeedRnd)) << 12;
		foo = __nw__4CBitUi(0x70);

		if (foo)
			foo =
			    __11CMotionBlurRC7CVectorR7CVectorPciii(foo, mPos, &upvel, s_Smoke, mBlurLife + Rnd(mBlurLifeRnd), 0, 10);

		foo->SetM40(100);
		foo->SetM60(Rnd(4096));
		SetTint__7CFT4BitUcUcUc(foo, mR, mG, mB);
	}

	{
		short l;

		mLife++;
		l = mLife;

		if (l == mMaxLife && (unsigned short)l != 0xffff)
			Die__4CBit(this);
	}
}

/* FUN_80060e38 (0x80060e38) */
struct Q {
	int a[4];
};

struct B {
	struct Q a;
	struct Q b;
};

extern struct B DAT_800bf260;
extern struct B DAT_800bf280;

void FUN_80060e38(void) {
	DAT_800bf280.a = DAT_800bf260.a;
	DAT_800bf280.b = DAT_800bf260.b;
}

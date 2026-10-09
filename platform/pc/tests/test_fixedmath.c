/* C reimplementations of the hand-written fixed-point helpers (decomp/reimpl.tsv), checked against
 * hand-worked values and, differentially, against the retail MIPS run by mips_ref.c.
 *   gcc -m32 -O2 -std=gnu11 test_fixedmath.c mips_ref.c ../../../decomp/src/init/mips_*.c -o test_fixedmath
 *   ./test_fixedmath [path/to/SLUS_008.60]   (default ../../../extern/thps1_retail/SLUS_008.60)
 * Without the executable only the hand-worked checks run. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mips_ref.h"

struct CVector {
	int vx, vy, vz;
};

int Mips_CVecDotProd__FP7CVectorT0(struct CVector *a, struct CVector *b);
int Mips_FixMul__Fii(int a, int b);
int Mips_DotProd2(int a, int b, int c, int d);
int Mips_DotProd2Sign(int a, int b, int c, int d);

#define ADDR_DOTPROD2SIGN 0x80072038u
#define ADDR_FIXMUL 0x80072090u
#define ADDR_DOTPROD2 0x800720acu
#define ADDR_CVECDOTPROD 0x800720ecu
#define VEC_A 0x80100000u /* scratch RAM for the reference machine's arguments */
#define VEC_B 0x80100010u

#define RANDOM_CASES 400000

static int failures, checks;

#define CHECK_EQ(actual, expected)                                                                 \
	do {                                                                                           \
		long long a_ = (long long)(actual), e_ = (long long)(expected);                            \
		checks++;                                                                                  \
		if (a_ != e_) {                                                                            \
			failures++;                                                                            \
			printf("%s:%d: %s = 0x%llx (%lld), expected 0x%llx (%lld)\n", __FILE__, __LINE__,      \
			       #actual, a_, a_, e_, e_);                                                       \
		}                                                                                          \
	} while (0)

static int Dot(int ax, int ay, int az, int bx, int by, int bz) {
	struct CVector a = {ax, ay, az}, b = {bx, by, bz};
	return Mips_CVecDotProd__FP7CVectorT0(&a, &b);
}

/* Expected values worked by hand: 4096 is 1.0. 65535 * 65537 = 0xffffffff, which plus 1 carries
 * into HI. */
static void TestHandWorked(void) {
	CHECK_EQ(Mips_FixMul__Fii(4096, 4096), 4096);                 /* 1 * 1 */
	CHECK_EQ(Mips_FixMul__Fii(8192, 2048), 4096);                 /* 2 * 0.5 */
	CHECK_EQ(Mips_FixMul__Fii(-4096, 4096), -4096);
	CHECK_EQ(Mips_FixMul__Fii(1, 1), 0);                          /* truncated */
	CHECK_EQ(Mips_FixMul__Fii(-1, 1), -1);                        /* the shift floors */
	CHECK_EQ(Mips_FixMul__Fii(0x10000, 0x10000), 0x100000);       /* 2^32 >> 12 */
	CHECK_EQ(Mips_FixMul__Fii(INT32_MIN, 1), -0x80000);           /* -2^31 >> 12 */
	CHECK_EQ(Mips_FixMul__Fii(INT32_MIN, INT32_MIN), 0);          /* 2^62 >> 12 = 2^50 */
	/* (2^31-1)^2 = 0x3fffffff00000001; bits 12..43 = 0xfff00000 */
	CHECK_EQ(Mips_FixMul__Fii(INT32_MAX, INT32_MAX), (int)0xfff00000u);

	CHECK_EQ(Mips_DotProd2(4096, 4096, 4096, 4096), 8192);        /* 1 + 1 */
	CHECK_EQ(Mips_DotProd2(4096, 4096, -4096, 4096), 0);          /* 1 - 1 */
	CHECK_EQ(Mips_DotProd2(8192, 2048, 4096, -2048), 2048);       /* 1 - 0.5 */
	CHECK_EQ(Mips_DotProd2(1, 2, 3, 4), 0);                       /* 14 / 4096: truncated */
	CHECK_EQ(Mips_DotProd2(-1, 1, 0, 0), -1);
	CHECK_EQ(Mips_DotProd2(65535, 65537, 1, 1), 0x100000);        /* 2^32 >> 12, via the carry */
	CHECK_EQ(Mips_DotProd2(INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN), 0); /* 2^63 */
	/* 2 * 0x3fffffff00000001 = 0x7ffffffe00000002; bits 12..43 = 0xffe00000 */
	CHECK_EQ(Mips_DotProd2(INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX), (int)0xffe00000u);

	CHECK_EQ(Mips_DotProd2Sign(0, 0, 0, 0), 0);
	CHECK_EQ(Mips_DotProd2Sign(1, 1, 0, 0), 1);
	CHECK_EQ(Mips_DotProd2Sign(-1, 1, 0, 0), -1);
	CHECK_EQ(Mips_DotProd2Sign(1, 1, -1, 1), 0);                  /* cancels exactly */
	CHECK_EQ(Mips_DotProd2Sign(1, 2, 0, 0), 1);                   /* below the 20.12 resolution */
	CHECK_EQ(Mips_DotProd2Sign(65535, 65537, 1, 1), 1);           /* 2^32: LO is 0, HI is 1 */
	CHECK_EQ(Mips_DotProd2Sign(-65536, 65536, 0, 0), -1);         /* -2^32: LO is 0 */
	CHECK_EQ(Mips_DotProd2Sign(INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN), -1); /* 2^63 wraps */
	CHECK_EQ(Mips_DotProd2Sign(INT32_MIN, INT32_MIN, INT32_MIN, 1), 1);

	CHECK_EQ(Dot(0, 0, 0, 0, 0, 0), 0);
	CHECK_EQ(Dot(4096, 0, 0, 4096, 0, 0), 4096);                /* 1 * 1 */
	CHECK_EQ(Dot(4096, 0, 0, 0, 4096, 0), 0);                   /* perpendicular */
	CHECK_EQ(Dot(-4096, 0, 0, 4096, 0, 0), -4096);              /* -1 * 1 */
	CHECK_EQ(Dot(8192, 4096, -4096, 4096, 4096, 4096), 8192);   /* 2 + 1 - 1 */
	CHECK_EQ(Dot(2048, 2048, 2048, 2048, 2048, 2048), 3072);    /* 3 * 0.25 */
	CHECK_EQ(Dot(1, 2, 3, 4, 5, 6), 0);                         /* 32 / 4096: truncated */
	CHECK_EQ(Dot(-1, 0, 0, 1, 0, 0), -1);                       /* the shift floors, not toward 0 */
	CHECK_EQ(Dot(0x10000, 0, 0, 0x10000, 0, 0), 0x100000);      /* 2^32 >> 12: carries into HI */
	CHECK_EQ(Dot(0x40000000, 0, 0, 0x40000000, 0, 0), 0);       /* 2^60 >> 12 = 2^48: truncated */
	/* 3 * 2^62 wraps the 64-bit sum to 0xc000000000000000; bits 12..43 are 0 */
	CHECK_EQ(Dot(INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN), 0);
	/* (2^31-1)^2 * 3 = 0xbffffffd00000003; bits 12..43 = 0xffd00000 */
	CHECK_EQ(Dot(INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX), (int)0xffd00000u);
}

static uint32_t rng = 0x2545f491u;

static uint32_t Rand(void) {
	rng ^= rng << 13;
	rng ^= rng >> 17;
	rng ^= rng << 5;
	return rng;
}

/* Arguments that stress different parts of the arithmetic. */
static int RandArg(int kind) {
	static const int32_t edges[] = {0, 1, -1, 4096, -4096, 0xfff, 0x1000, INT32_MAX, INT32_MIN,
	                                0x7fff, -0x8000, 0xffff, 0x10000, 0x10001, -0x10000,
	                                0x40000000, (int32_t)0xc0000000};
	switch (kind) {
	case 0: return (int32_t)Rand();                                   /* full range */
	case 1: return (int32_t)Rand() >> 12;                             /* game range: +-2^19 units */
	case 2: return (int32_t)(Rand() & 0x1fff) - 0x1000;               /* unit vectors */
	case 3: return (int32_t)(Rand() & 0xf) - 8;                       /* tiny: sums near zero */
	default: return edges[Rand() % (sizeof edges / sizeof edges[0])];
	}
}

static int RefDot(MRef *m, const struct CVector *a, const struct CVector *b) {
	memcpy(MRef_Ptr(m, VEC_A), a, sizeof *a);
	memcpy(MRef_Ptr(m, VEC_B), b, sizeof *b);
	return (int)MRef_Call(m, ADDR_CVECDOTPROD, VEC_A, VEC_B, 0, 0);
}

/* The executable has the routines where functions.tsv says (first two words of each). */
static int CheckLayout(MRef *m) {
	static const struct {
		uint32_t addr, w0, w1;
	} fns[] = {
	    {ADDR_DOTPROD2SIGN, 0x00850018u, 0x00001012u}, /* mult a0,a1; mflo v0 */
	    {ADDR_FIXMUL, 0x00850018u, 0x00001012u},
	    {ADDR_DOTPROD2, 0x00850018u, 0x00001012u},
	    {ADDR_CVECDOTPROD, 0x8c880000u, 0x8ca90000u}, /* lw t0,0(a0); lw t1,0(a1) */
	};
	unsigned i;
	for (i = 0; i < sizeof fns / sizeof fns[0]; i++) {
		uint32_t w[2] = {fns[i].w0, fns[i].w1};
		if (memcmp(MRef_Ptr(m, fns[i].addr), w, sizeof w)) {
			printf("retail: unexpected code at %08x: wrong executable?\n", fns[i].addr);
			failures++;
			return 0;
		}
	}
	return 1;
}

#define REPORT(name, fmt, ...)                                                                     \
	do {                                                                                           \
		checks++;                                                                                  \
		if (got != want && failures++ - n_fail < 10)                                               \
			printf("retail mismatch: " name fmt ": C %d, retail %d\n", __VA_ARGS__, got, want);    \
	} while (0)

static void TestAgainstRetail(MRef *m) {
	struct CVector a, b;
	int i, n_fail = failures;

	if (!CheckLayout(m))
		return;
	/* hand-worked cases through the retail code: checks the expectations and mips_ref.c */
	CHECK_EQ((int)MRef_Call(m, ADDR_FIXMUL, (uint32_t)INT32_MAX, (uint32_t)INT32_MAX, 0, 0),
	         (int)0xfff00000u);
	CHECK_EQ((int)MRef_Call(m, ADDR_FIXMUL, (uint32_t)-1, 1, 0, 0), -1);
	CHECK_EQ((int)MRef_Call(m, ADDR_DOTPROD2, 65535, 65537, 1, 1), 0x100000);
	CHECK_EQ((int)MRef_Call(m, ADDR_DOTPROD2, (uint32_t)INT32_MAX, (uint32_t)INT32_MAX,
	                        (uint32_t)INT32_MAX, (uint32_t)INT32_MAX), (int)0xffe00000u);
	CHECK_EQ((int)MRef_Call(m, ADDR_DOTPROD2SIGN, 1, 2, 0, 0), 1);
	CHECK_EQ((int)MRef_Call(m, ADDR_DOTPROD2SIGN, (uint32_t)-65536, 65536, 0, 0), -1);
	CHECK_EQ((int)MRef_Call(m, ADDR_DOTPROD2SIGN, 1, 1, (uint32_t)-1, 1), 0);
	a = (struct CVector){8192, 4096, -4096};
	b = (struct CVector){4096, 4096, 4096};
	CHECK_EQ(RefDot(m, &a, &b), 8192);
	a = (struct CVector){-1, 0, 0};
	b = (struct CVector){1, 0, 0};
	CHECK_EQ(RefDot(m, &a, &b), -1);
	a = (struct CVector){INT32_MAX, INT32_MAX, INT32_MAX};
	CHECK_EQ(RefDot(m, &a, &a), (int)0xffd00000u);

	for (i = 0; i < RANDOM_CASES; i++) {
		int kind = i % 5, got, want;
		int p = RandArg(kind), q = RandArg(kind), r = RandArg(kind), s = RandArg(kind);

		want = (int)MRef_Call(m, ADDR_FIXMUL, p, q, 0, 0);
		got = Mips_FixMul__Fii(p, q);
		REPORT("Mips_FixMul", "(%d,%d)", p, q);

		want = (int)MRef_Call(m, ADDR_DOTPROD2, p, q, r, s);
		got = Mips_DotProd2(p, q, r, s);
		REPORT("Mips_DotProd2", "(%d,%d,%d,%d)", p, q, r, s);

		/* half the sign cases cancel exactly, so 0 is exercised as much as -1 and 1 */
		if (i & 1)
			r = -p, s = q;
		want = (int)MRef_Call(m, ADDR_DOTPROD2SIGN, p, q, r, s);
		got = Mips_DotProd2Sign(p, q, r, s);
		REPORT("Mips_DotProd2Sign", "(%d,%d,%d,%d)", p, q, r, s);

		a = (struct CVector){RandArg(kind), RandArg(kind), RandArg(kind)};
		b = (struct CVector){RandArg(kind), RandArg(kind), RandArg(kind)};
		want = RefDot(m, &a, &b);
		got = Mips_CVecDotProd__FP7CVectorT0(&a, &b);
		REPORT("Mips_CVecDotProd", "((%d,%d,%d),(%d,%d,%d))", a.vx, a.vy, a.vz, b.vx, b.vy, b.vz);
	}
}

int main(int argc, char **argv) {
	const char *exe = argc > 1 ? argv[1] : "../../../extern/thps1_retail/SLUS_008.60";
	MRef m;

	TestHandWorked();
	if (MRef_Load(&m, exe)) {
		TestAgainstRetail(&m);
		MRef_Free(&m);
	} else {
		printf("SKIPPED the retail comparison: can't load %s\n", exe);
	}
	printf("%d/%d checks passed\n", checks - failures, checks);
	return failures != 0;
}

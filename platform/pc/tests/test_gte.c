/* Software GTE tests: expected values are worked by hand from the psx-spx formulas.
 *   gcc -m32 -std=gnu11 -I../include -I../include/psyq test_gte.c ../src/pc_gte.c -o test_gte */
#include <stdio.h>
#include <stdlib.h>

#include "pc_gte.h"
#include <sys/types.h>
#include <libgte.h>
#include <inline_c.h>

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

#define D(r) PC_GteGetData(r)
#define C(r) PC_GteGetCtrl(r)
#define SETD(r, v) PC_GteSetData(r, (uint32_t)(v))
#define SETC(r, v) PC_GteSetCtrl(r, (uint32_t)(v))
#define XY(x, y) (((uint32_t)(uint16_t)(x)) | ((uint32_t)(uint16_t)(y) << 16))

/* command words: sf (bit 19), lm (bit 10) variants */
#define CMD(op, sf, lm) PC_GteCommand((op) | ((sf) << 19) | ((lm) << 10))
#define RTPS 0x01
#define NCLIP 0x06
#define OP 0x0C
#define DPCS 0x10
#define INTPL 0x11
#define NCDS 0x13
#define CDP 0x14
#define NCDT 0x16
#define NCCS 0x1B
#define CC 0x1C
#define NCS 0x1E
#define NCT 0x20
#define SQR 0x28
#define DCPL 0x29
#define DPCT 0x2A
#define GPF 0x3D
#define GPL 0x3E
#define NCCT 0x3F

static void set_identity_rt(void)
{
	SETC(0, 0x1000);
	SETC(1, 0);
	SETC(2, 0x1000);
	SETC(3, 0);
	SETC(4, 0x1000);
}

static void test_registers(void)
{
	PC_GteReset();
	/* V0..V2: XY packed, Z sign-extended */
	SETD(0, 0x8001FFFF);
	CHECK_EQ(D(0), 0x8001FFFF);
	SETD(1, 0x0001FFFF);
	CHECK_EQ(D(1), 0xFFFFFFFF);
	SETD(5, 0x12347FFF);
	CHECK_EQ(D(5), 0x7FFF);
	/* RGBC full word */
	SETD(6, 0x30112233);
	CHECK_EQ(D(6), 0x30112233);
	/* OTZ and SZ zero-extend */
	SETD(7, 0xFFFF1234);
	CHECK_EQ(D(7), 0x1234);
	SETD(17, 0x1234F678);
	CHECK_EQ(D(17), 0xF678);
	/* IR0-3 sign-extend */
	SETD(8, 0x00008000);
	CHECK_EQ(D(8), 0xFFFF8000);
	SETD(11, 0xABCD7FFF);
	CHECK_EQ(D(11), 0x7FFF);
	/* MAC, RGB FIFO and RES1 are full words */
	SETD(25, 0x80000001);
	CHECK_EQ(D(25), 0x80000001);
	SETD(23, 0xDEADBEEF);
	CHECK_EQ(D(23), 0xDEADBEEF);

	/* SXY FIFO: SXYP pushes, reads back SXY2 */
	SETD(12, XY(1, -1));
	SETD(13, XY(2, -2));
	SETD(14, XY(3, -3));
	CHECK_EQ(D(15), XY(3, -3));
	SETD(15, XY(4, -4));
	CHECK_EQ(D(12), XY(2, -2));
	CHECK_EQ(D(13), XY(3, -3));
	CHECK_EQ(D(14), XY(4, -4));
	CHECK_EQ(D(15), XY(4, -4));

	/* IRGB write expands 5:5:5 into IR1-3; IRGB/ORGB read packs IR>>7 saturated */
	SETD(28, 0x7FFF);
	CHECK_EQ(D(9), 0xF80);
	CHECK_EQ(D(10), 0xF80);
	CHECK_EQ(D(11), 0xF80);
	SETD(28, (3 << 10) | (2 << 5) | 1);
	CHECK_EQ(D(9), 0x80);
	CHECK_EQ(D(10), 0x100);
	CHECK_EQ(D(11), 0x180);
	CHECK_EQ(D(29), (3 << 10) | (2 << 5) | 1);
	SETD(9, 0x1000);  /* >>7 = 0x20 -> 0x1F */
	SETD(10, -5);     /* negative -> 0 */
	SETD(11, 0x100);  /* -> 2 */
	CHECK_EQ(D(29), 0x1F | (0 << 5) | (2 << 10));
	CHECK_EQ(D(28), 0x1F | (0 << 5) | (2 << 10));
	SETD(29, 0x1234); /* ORGB is read-only */
	CHECK_EQ(D(9), 0x1000);

	/* LZCS / LZCR */
	static const uint32_t lz_in[] = {0, 0xFFFFFFFF, 1, 0x80000000, 0x00FF0000, 0xF0000000,
	                                 0x7FFFFFFF, 0xFFFF0000, 0x00008000};
	static const uint32_t lz_out[] = {32, 32, 31, 1, 8, 4, 1, 16, 16};
	for (unsigned i = 0; i < sizeof lz_in / sizeof lz_in[0]; i++) {
		SETD(30, lz_in[i]);
		CHECK_EQ(D(31), lz_out[i]);
		CHECK_EQ(D(30), lz_in[i]);
	}
	SETD(31, 5); /* LZCR is read-only */
	CHECK_EQ(D(31), 16);

	/* control: matrix words, 16-bit singles sign-extend, H sign-extends on read */
	SETC(0, 0x80001234);
	CHECK_EQ(C(0), 0x80001234);
	SETC(4, 0x1234FFFF);
	CHECK_EQ(C(4), 0xFFFFFFFF);
	SETC(12, 0x00008000);
	CHECK_EQ(C(12), 0xFFFF8000);
	SETC(20, 0x00007000);
	CHECK_EQ(C(20), 0x7000);
	SETC(7, 0x80000000);
	CHECK_EQ(C(7), 0x80000000);
	SETC(26, 0x00008000);
	CHECK_EQ(C(26), 0xFFFF8000);
	SETC(27, 0x00018000);
	CHECK_EQ(C(27), 0xFFFF8000);
	SETC(29, 0x0000FFFF);
	CHECK_EQ(C(29), 0xFFFFFFFF);
	SETC(30, 0x00000400);
	CHECK_EQ(C(30), 0x400);
	SETC(28, 0x89ABCDEF);
	CHECK_EQ(C(28), 0x89ABCDEF);

	/* FLAG: bits 0-11 read 0, bit 31 is the summary of bits 30-23 and 18-13 */
	SETC(31, 0xFFFFFFFF);
	CHECK_EQ(C(31), 0xFFFFF000);
	SETC(31, 0x00001000); /* IR0 is not an error bit */
	CHECK_EQ(C(31), 0x00001000);
	SETC(31, 0x00400000); /* IR3 is not an error bit */
	CHECK_EQ(C(31), 0x00400000);
	SETC(31, 0x00040000); /* SZ3/OTZ is */
	CHECK_EQ(C(31), 0x80040000);
	SETC(31, 0x00800000); /* IR2 is */
	CHECK_EQ(C(31), 0x80800000);
	SETC(31, 0x00380000); /* colour FIFO bits are not */
	CHECK_EQ(C(31), 0x00380000);

	PC_GteReset();
	CHECK_EQ(D(6), 0);
	CHECK_EQ(C(31), 0);
}

/* projection: RT = identity, TR = 0, H = 1000, OF = (160, 120), DQA = -256, DQB = 0x1400000 */
static void setup_projection(void)
{
	PC_GteReset();
	set_identity_rt();
	SETC(26, 1000);
	SETC(24, 160 << 16);
	SETC(25, 120 << 16);
	SETC(27, -256);
	SETC(28, 0x1400000);
}

static void test_rtps(void)
{
	setup_projection();
	SETD(0, XY(100, 50));
	SETD(1, 1000);
	SETD(19, 77); /* old SZ3 moves into SZ2 */
	gte_rtps();
	CHECK_EQ(C(31), 0);
	CHECK_EQ(D(25), 100);
	CHECK_EQ(D(26), 50);
	CHECK_EQ(D(27), 1000);
	CHECK_EQ(D(9), 100);
	CHECK_EQ(D(10), 50);
	CHECK_EQ(D(11), 1000);
	CHECK_EQ(D(19), 1000);
	CHECK_EQ(D(18), 77);
	/* H == SZ3: UNR gives exactly 1.0 (0x10000) */
	CHECK_EQ(D(14), XY(260, 170));
	CHECK_EQ(D(24), 0x400000); /* -256*0x10000 + 0x1400000 */
	CHECK_EQ(D(8), 0x400);

	/* translation and negative coordinates; z = 2000 -> H/SZ3 = 0.5 */
	setup_projection();
	SETC(5, -50);
	SETC(6, 10);
	SETC(7, 1000);
	SETD(0, XY(-150, -30));
	SETD(1, 1000);
	gte_rtps();
	CHECK_EQ(C(31), 0);
	CHECK_EQ(D(19), 2000);
	/* n = 0x8000: sx = (0x8000*-200 + 160<<16)>>16 = 60, sy = (0x8000*-20 + 120<<16)>>16 = 110 */
	CHECK_EQ(D(14), XY(60, 110));
	CHECK_EQ(D(24), -256 * 0x8000 + 0x1400000);
	CHECK_EQ(D(8), (-256 * 0x8000 + 0x1400000) >> 12);

	/* divide overflow: SZ3 = 10, H >= 2*SZ3 -> n = 0x1FFFF, FLAG.17; IR0 negative -> 0, FLAG.12 */
	setup_projection();
	SETD(0, XY(100, 50));
	SETD(1, 10);
	gte_rtps();
	CHECK_EQ(D(19), 10);
	CHECK_EQ(D(14), XY((0x1FFFF * 100 + (160 << 16)) >> 16, (0x1FFFF * 50 + (120 << 16)) >> 16));
	CHECK_EQ((int32_t)D(24), 0x1FFFF * -256 + 0x1400000);
	CHECK_EQ(D(8), 0);
	CHECK_EQ(C(31), 0x80000000 | (1 << 17) | (1 << 12));

	/* screen X/Y saturation (-400h..3FFh) */
	setup_projection();
	SETD(0, XY(2000, -3000));
	SETD(1, 1000);
	gte_rtps();
	CHECK_EQ(D(14), XY(0x3FF, -0x400));
	CHECK_EQ(C(31), 0x80000000 | (1 << 14) | (1 << 13));

	/* negative Z: SZ3 clamps to 0 (FLAG.18) and the divide overflows (FLAG.17) */
	setup_projection();
	SETD(0, XY(0, 0));
	SETD(1, -1000);
	gte_rtps();
	CHECK_EQ(D(19), 0);
	CHECK_EQ(D(11), 0xFFFFFC18);
	CHECK_EQ(C(31), 0x80000000 | (1 << 18) | (1 << 17) | (1 << 12));

	/* sf = 0: MAC unshifted; IR1 saturates with flag; IR3 saturates but its flag follows MAC3>>12 */
	setup_projection();
	SETD(0, XY(100, 0));
	SETD(1, 1000);
	CMD(RTPS, 0, 0);
	CHECK_EQ(D(25), 100 * 4096);
	CHECK_EQ(D(27), 1000 * 4096);
	CHECK_EQ(D(9), 0x7FFF);
	CHECK_EQ(D(11), 0x7FFF);
	CHECK_EQ(D(19), 1000);
	CHECK_EQ(D(14), XY(0x3FF, 120)); /* projected from the saturated IR1 */
	/* 0x10000 * 0x7FFF + OFX also overflows MAC0 (FLAG.16) */
	CHECK_EQ(C(31), 0x80000000 | (1 << 24) | (1 << 16) | (1 << 14));

	/* MAC3 >> 12 outside 16 bits: IR3 flag set (sf = 1), SZ3 saturates */
	setup_projection();
	SETC(27, 0);
	SETC(28, 0);
	SETC(7, 40000);
	SETD(0, 0);
	SETD(1, 0);
	gte_rtps();
	CHECK_EQ(D(27), 40000);
	CHECK_EQ(D(11), 0x7FFF);
	CHECK_EQ(D(19), 40000);
	CHECK_EQ(C(31), 0x00400000);
	SETC(7, 70000);
	gte_rtps();
	CHECK_EQ(D(19), 0xFFFF);
	CHECK_EQ(C(31), 0x80000000 | (1 << 22) | (1 << 18));
}

static void test_rtpt(void)
{
	setup_projection();
	SETD(0, XY(100, 0));
	SETD(1, 1000);
	SETD(2, XY(0, 100));
	SETD(3, 2000);
	SETD(4, XY(-100, -100));
	SETD(5, 4000);
	SETD(19, 55);
	SETC(28, 0); /* keep IR0 in range */
	gte_rtpt();
	CHECK_EQ(D(16), 55);
	CHECK_EQ(D(17), 1000);
	CHECK_EQ(D(18), 2000);
	CHECK_EQ(D(19), 4000);
	CHECK_EQ(D(12), XY(260, 120));
	CHECK_EQ(D(13), XY(160, 170));
	CHECK_EQ(D(14), XY(135, 95));
	/* IR0/MAC0 from the last vertex only: n = 0x4000 */
	CHECK_EQ((int32_t)D(24), -256 * 0x4000);
	CHECK_EQ(D(8), 0);
	CHECK_EQ(C(31), 1 << 12); /* IR0 is not an error bit */
	/* the IR registers hold the last vertex */
	CHECK_EQ(D(9), -100 & 0xFFFFFFFF);
	CHECK_EQ(D(11), 4000);
}

static void test_divide(void)
{
	/* spot values */
	static const struct {
		uint16_t h, sz;
		uint32_t n;
	} spot[] = {
	    {1000, 1000, 0x10000}, {0x8000, 0x8000, 0x10000}, {1, 0xFFFF, 1},
	    {0, 5, 0},             {1000, 2000, 0x8000},      {1000, 4000, 0x4000},
	    {1000, 500, 0x1FFFF},  {1000, 501, 130811},       {0xFFFF, 0xFFFF, 0xFFFF},
	    {5, 3, 109226},        {7605, 4090, 121856},      {0xFFFF, 0x8000, 0x1FFFE},
	};
	for (unsigned i = 0; i < sizeof spot / sizeof spot[0]; i++) {
		PC_GteReset();
		SETC(26, spot[i].h);
		SETC(27, 1); /* MAC0 = n */
		SETC(7, spot[i].sz);
		set_identity_rt();
		gte_rtps();
		CHECK_EQ(D(19), spot[i].sz);
		CHECK_EQ(D(24), spot[i].n);
		CHECK_EQ(C(31) & (1 << 17), spot[i].h >= spot[i].sz * 2 ? (1 << 17) : 0);
	}

	/* sweep: UNR stays within a few units of the rounded quotient (it is up to 3 off, e.g. 7605/4090
	 * above), and is exact for powers of two */
	int worst = 0;
	for (uint32_t sz = 1; sz <= 0xFFFF; sz += 7) {
		uint32_t hs[4] = {0, sz / 3, sz, sz * 2 - 1};
		for (int k = 0; k < 4; k++) {
			uint32_t h = hs[k];
			if (h > 0xFFFF)
				continue;
			PC_GteReset();
			SETC(26, h);
			SETC(28, 0);
			SETC(27, 1); /* MAC0 = n */
			SETC(7, sz);
			set_identity_rt();
			gte_rtps();
			int64_t n = (int32_t)D(24);
			int64_t exact = (((int64_t)h * 0x20000 / sz) + 1) / 2;
			if (exact > 0x1FFFF)
				exact = 0x1FFFF;
			int diff = (int)(n > exact ? n - exact : exact - n);
			if (diff > worst)
				worst = diff;
		}
	}
	CHECK_EQ(worst <= 3, 1);
	for (int s = 0; s < 16; s++) {
		PC_GteReset();
		SETC(26, 0x7FFF >> (15 - s) | 1);
		SETC(27, 1);
		SETC(7, 1u << s);
		set_identity_rt();
		uint32_t h = (0x7FFF >> (15 - s) | 1);
		gte_rtps();
		uint32_t expect = h < (2u << s) ? (uint32_t)(((uint64_t)h << 16) >> s) : 0x1FFFF;
		if (expect > 0x1FFFF)
			expect = 0x1FFFF;
		CHECK_EQ(D(24), expect);
	}
}

static void test_nclip_avsz(void)
{
	PC_GteReset();
	SETD(12, XY(0, 0));
	SETD(13, XY(10, 0));
	SETD(14, XY(0, 10));
	gte_nclip();
	CHECK_EQ((int32_t)D(24), 100);
	CHECK_EQ(C(31), 0);
	SETD(13, XY(0, 10));
	SETD(14, XY(10, 0));
	gte_nclip();
	CHECK_EQ((int32_t)D(24), -100);

	/* MAC0 positive overflow */
	SETD(12, XY(0x7FFF, -0x8000));
	SETD(13, XY(0x7FFF, 0x7FFF));
	SETD(14, XY(-0x8000, 0x7FFF));
	gte_nclip();
	CHECK_EQ(D(24), 0xFFFE0001);
	CHECK_EQ(C(31), 0x80010000);

	/* MAC0 negative overflow (mirror image) */
	SETD(13, XY(-0x8000, 0x7FFF));
	SETD(14, XY(0x7FFF, 0x7FFF));
	SETD(12, XY(0x7FFF, -0x8000));
	gte_nclip();
	CHECK_EQ(C(31) & 0x80018000, 0x80008000);

	/* AVSZ3 / AVSZ4 */
	PC_GteReset();
	SETD(16, 100);
	SETD(17, 100);
	SETD(18, 200);
	SETD(19, 300);
	SETC(29, 0x555);
	gte_avsz3();
	CHECK_EQ(D(24), 0x555 * 600);
	CHECK_EQ(D(7), (0x555 * 600) >> 12);
	CHECK_EQ(C(31), 0);
	SETD(16, 400);
	SETC(30, 0x400);
	gte_avsz4();
	CHECK_EQ(D(24), 0x400 * 1000);
	CHECK_EQ(D(7), 250);

	/* OTZ saturation high (and MAC0 overflow), and low */
	SETD(17, 0xFFFF);
	SETD(18, 0xFFFF);
	SETD(19, 0xFFFF);
	SETC(29, 0x7FFF);
	gte_avsz3();
	CHECK_EQ(D(24), 2147188739u);
	CHECK_EQ(D(7), 0xFFFF);
	CHECK_EQ(C(31), 0x80000000 | (1 << 18) | (1 << 16));
	SETD(17, 100);
	SETD(18, 100);
	SETD(19, 100);
	SETC(29, -0x1000);
	gte_avsz3();
	CHECK_EQ((int32_t)D(24), -0x1000 * 300);
	CHECK_EQ(D(7), 0);
	CHECK_EQ(C(31), 0x80040000);
}

static void test_op_sqr(void)
{
	/* OP: D = diag(RT); x cross y = z */
	PC_GteReset();
	SETC(0, 0x1000);
	SETC(2, 0);
	SETC(4, 0);
	SETD(9, 0);
	SETD(10, 0x1000);
	SETD(11, 0);
	gte_op12();
	CHECK_EQ(D(25), 0);
	CHECK_EQ(D(26), 0);
	CHECK_EQ(D(27), 0x1000);
	CHECK_EQ(D(11), 0x1000);
	CHECK_EQ(C(31), 0);
	/* sf = 0: IR3 saturates; FLAG.22 alone does not set bit 31 */
	SETD(9, 0);
	SETD(10, 0x1000);
	SETD(11, 0);
	gte_op0();
	CHECK_EQ(D(27), 0x1000000);
	CHECK_EQ(D(11), 0x7FFF);
	CHECK_EQ(C(31), 0x00400000);
	/* lm = 1 clamps negatives to 0 */
	SETD(9, 0);
	SETD(10, -0x1000);
	SETD(11, 0);
	CMD(OP, 1, 1);
	CHECK_EQ((int32_t)D(27), -0x1000);
	CHECK_EQ(D(11), 0);
	CHECK_EQ(C(31), 0x00400000);
	/* full cross product: D = (1,2,3), IR = (4,5,6) -> (2*6-3*5, 3*4-1*6, 1*5-2*4) */
	SETC(0, 1);
	SETC(2, 2);
	SETC(4, 3);
	SETD(9, 4);
	SETD(10, 5);
	SETD(11, 6);
	gte_op0();
	CHECK_EQ((int32_t)D(25), -3);
	CHECK_EQ((int32_t)D(26), 6);
	CHECK_EQ((int32_t)D(27), -3);

	/* SQR */
	SETD(9, 3);
	SETD(10, -4);
	SETD(11, 0x200);
	gte_sqr0();
	CHECK_EQ(D(25), 9);
	CHECK_EQ(D(26), 16);
	CHECK_EQ(D(27), 0x40000);
	CHECK_EQ(D(11), 0x7FFF);
	CHECK_EQ(C(31), 0x00400000);
	SETD(9, 0x1000);
	SETD(10, 0x800);
	SETD(11, -0x2000);
	gte_sqr12();
	CHECK_EQ(D(25), 0x1000);
	CHECK_EQ(D(26), 0x400);
	CHECK_EQ(D(27), 0x4000);
	CHECK_EQ(C(31), 0);
}

static void test_mvmva(void)
{
	PC_GteReset();
	/* RT = diag(1, 0.5, 2), TR = (10, 20, 30), BK = (1, 2, 3), V1 = (100, 200, 300) */
	SETC(0, 0x1000);
	SETC(1, 0);
	SETC(2, 0x800);
	SETC(3, 0);
	SETC(4, 0x2000);
	SETC(5, 10);
	SETC(6, 20);
	SETC(7, 30);
	SETC(13, 1);
	SETC(14, 2);
	SETC(15, 3);
	SETD(2, XY(100, 200));
	SETD(3, 300);
	gte_rtv1tr();
	CHECK_EQ(D(25), 110);
	CHECK_EQ(D(26), 120);
	CHECK_EQ(D(27), 630);
	CHECK_EQ(D(11), 630);
	gte_rtv1bk();
	CHECK_EQ(D(25), 101);
	CHECK_EQ(D(26), 102);
	CHECK_EQ(D(27), 603);
	gte_rtv1();
	CHECK_EQ(D(25), 100);
	CHECK_EQ(D(26), 100);
	CHECK_EQ(D(27), 600);
	/* v = IR: IR now (100, 100, 600) */
	gte_rtirtr();
	CHECK_EQ(D(25), 110);
	CHECK_EQ(D(26), 70);
	CHECK_EQ(D(27), 1230);
	/* sf = 0 */
	gte_rtir_sf0();
	CHECK_EQ(D(25), 110 * 4096);
	CHECK_EQ(D(9), 0x7FFF);
	CHECK_EQ(C(31), 0x80000000 | (7u << 22));

	/* LLM and LCM selection */
	SETC(8, 0x2000);
	SETC(9, 0);
	SETC(10, 0x2000);
	SETC(11, 0);
	SETC(12, 0x2000);
	SETC(16, 0x400);
	SETC(17, 0);
	SETC(18, 0x400);
	SETC(19, 0);
	SETC(20, 0x400);
	gte_llv1();
	CHECK_EQ(D(25), 200);
	CHECK_EQ(D(27), 600);
	gte_lcv1();
	CHECK_EQ(D(25), 25);
	CHECK_EQ(D(27), 75);
	/* lm = 1 */
	SETD(2, XY(-100, 200));
	gte_mvmva(1, 0, 1, 3, 1);
	CHECK_EQ((int32_t)D(25), -100);
	CHECK_EQ(D(9), 0);
	CHECK_EQ(C(31), 0x81000000);

	/* garbage matrix (mx = 3): rows (-R<<4, R<<4, IR0), (RT13 x3), (RT22 x3) */
	PC_GteReset();
	SETD(6, 0x10);
	SETD(8, 0x100);
	SETC(1, 0x1000); /* RT13 = 0x1000 */
	SETC(2, 0x800);  /* RT22 = 0x800 */
	SETD(0, XY(0x1000, 0x1000));
	SETD(1, 0x1000);
	gte_mvmva(1, 3, 0, 3, 0);
	CHECK_EQ(D(25), 0x100);
	CHECK_EQ(D(26), 0x3000);
	CHECK_EQ(D(27), 0x1800);

	/* cv = 2 (far colour) bug: only columns 2 and 3 reach the result; FC + column 1 sets flags */
	PC_GteReset();
	set_identity_rt();
	SETC(21, 0x100000);
	SETC(22, 0);
	SETC(23, 0);
	SETD(0, XY(5, 6));
	SETD(1, 7);
	gte_mvmva(1, 0, 0, 2, 0);
	CHECK_EQ(D(25), 0);
	CHECK_EQ(D(26), 6);
	CHECK_EQ(D(27), 7);
	CHECK_EQ(D(9), 0);
	CHECK_EQ(C(31), 0x81000000);

	/* 44-bit overflow on an intermediate step, with wrap */
	PC_GteReset();
	SETC(0, 0x7FFF);
	SETC(5, 0x7FFFFFFF);
	SETD(0, XY(0x7FFF, 0));
	SETD(1, 0);
	gte_rtv0tr();
	CHECK_EQ((int32_t)D(25), -0x7FFC0011);
	CHECK_EQ(D(9), 0xFFFF8000);
	CHECK_EQ(C(31), 0x80000000 | (1u << 30) | (1u << 24));
	/* negative overflow */
	SETC(5, 0x80000000);
	SETD(0, XY(-0x8000, 0));
	SETC(0, 0x7FFF);
	gte_rtv0tr();
	CHECK_EQ(C(31) & (1u << 27), 1u << 27);
}

/* light setup: LLM row 1 = x axis; LCM column 1 = (1.0, 0.5, 0.25); BK = (0x100, 0x200, 0x300) */
static void setup_lighting(void)
{
	PC_GteReset();
	SETC(8, 0x1000);
	SETC(9, 0);
	SETC(10, 0);
	SETC(11, 0);
	SETC(12, 0);
	SETC(16, 0x1000);
	SETC(17, 0x800 << 16);
	SETC(18, 0);
	SETC(19, 0x400);
	SETC(20, 0);
	SETC(13, 0x100);
	SETC(14, 0x200);
	SETC(15, 0x300);
	SETC(21, 0xFF0);
	SETC(22, 0);
	SETC(23, 0x800);
	SETD(6, 0x30204080);
	SETD(8, 0x800);
	SETD(0, XY(0x1000, 0));
	SETD(1, 0);
}

static void test_colour(void)
{
	/* NCS: colour = (BK + LCM*(LLM*V)) / 16 */
	setup_lighting();
	gte_ncs();
	CHECK_EQ(D(25), 0x1100);
	CHECK_EQ(D(26), 0xA00);
	CHECK_EQ(D(27), 0x700);
	CHECK_EQ(D(22), 0x3070A0FF);
	CHECK_EQ(C(31), 0x00200000);

	/* NCCS: multiplied by RGBC */
	setup_lighting();
	gte_nccs();
	CHECK_EQ(D(25), 0x880);
	CHECK_EQ(D(26), 0x280);
	CHECK_EQ(D(27), 0xE0);
	CHECK_EQ(D(22), 0x300E2888);
	CHECK_EQ(C(31), 0);

	/* NCDS: then depth-cued halfway to FC */
	setup_lighting();
	gte_ncds();
	CHECK_EQ(D(25), 0xC38);
	CHECK_EQ(D(26), 0x140);
	CHECK_EQ(D(27), 0x470);
	CHECK_EQ(D(22), 0x304714C3);
	CHECK_EQ(C(31), 0);

	/* NCT / NCCT / NCDT: three vertices into the colour FIFO */
	setup_lighting();
	SETD(2, XY(0x800, 0));
	SETD(3, 0);
	SETD(4, XY(-0x1000, 0));
	SETD(5, 0);
	gte_nct();
	CHECK_EQ(D(20), 0x3070A0FF);
	CHECK_EQ(D(21), 0x30506090); /* IR=(0x900,0x600,0x500) */
	CHECK_EQ(D(22), 0x30302010); /* LLM*V = -1 clamps to 0 (lm=1, FLAG.24) -> BK only */
	CHECK_EQ(C(31), 0x81200000);  /* flags accumulate over the three vertices */
	gte_ncct();
	CHECK_EQ(D(20), 0x300E2888);
	CHECK_EQ(D(21), 0x300A1848); /* (0x80*0x900, 0x40*0x600, 0x20*0x500) >> 8 */
	CHECK_EQ(D(22), 0x30060808);
	gte_ncdt();
	CHECK_EQ(D(22), 0x30430483); /* (0x08+0xFF)/2, (0x08+0)/2, (0x06+0x80)/2 */

	/* CC: IR -> BK + LCM*IR, times RGBC */
	PC_GteReset();
	SETC(16, 0x1000);
	SETC(18, 0x1000);
	SETC(20, 0x1000);
	SETD(6, 0x30204080);
	SETD(9, 0x1000);
	SETD(10, 0x800);
	SETD(11, 0);
	gte_cc();
	CHECK_EQ(D(22), 0x30002080);
	/* CDP: same, depth-cued with IR0 = 0 (no change) then IR0 = 1.0 (all FC) */
	SETD(9, 0x1000);
	SETD(10, 0x800);
	SETD(11, 0);
	SETD(8, 0);
	gte_cdp();
	CHECK_EQ(D(22), 0x30002080);
	SETC(21, 0xFF0);
	SETC(22, 0x100);
	SETC(23, 0x7F0);
	SETD(9, 0x1000);
	SETD(10, 0x800);
	SETD(11, 0);
	SETD(8, 0x1000);
	gte_cdp();
	CHECK_EQ(D(22), 0x307F10FF);

	/* DPCS: RGBC depth-cued halfway to FC */
	setup_lighting();
	gte_dpcs();
	CHECK_EQ(D(22), 0x305020BF);
	CHECK_EQ(C(31), 0);

	/* DPCT with IR0 = 0: copies the FIFO through, reading RGB0 each step; CODE from RGBC */
	PC_GteReset();
	SETD(6, 0x77000000);
	SETD(20, 0x01112233);
	SETD(21, 0x02445566);
	SETD(22, 0x03FF8800);
	gte_dpct();
	CHECK_EQ(D(20), 0x77112233);
	CHECK_EQ(D(21), 0x77445566);
	CHECK_EQ(D(22), 0x77FF8800);

	/* DCPL: RGBC * IR, IR0 = 0 */
	PC_GteReset();
	SETD(6, 0x00204080);
	SETD(9, 0x1000);
	SETD(10, 0x800);
	SETD(11, 0x400);
	gte_dpcl();
	CHECK_EQ(D(22), 0x00082080);

	/* INTPL: IR toward FC */
	PC_GteReset();
	SETD(9, 0x100);
	SETD(10, 0x200);
	SETD(11, 0x300);
	SETD(8, 0);
	gte_intpl();
	CHECK_EQ(D(22), 0x00302010);
	SETC(21, 0x1000);
	SETD(9, 0x100);
	SETD(10, 0x200);
	SETD(11, 0x300);
	SETD(8, 0x1000);
	gte_intpl();
	CHECK_EQ(D(25), 0x1000);
	CHECK_EQ(D(26), 0);
	CHECK_EQ(D(22), 0x000000FF);
	CHECK_EQ(C(31), 0x00200000);

	/* lm = 0 in the FC - MAC step even when the command has lm = 1 */
	PC_GteReset();
	SETD(9, 0x100);
	SETD(8, 0x800);
	CMD(INTPL, 1, 1);
	/* IR1 = (0 - 0x100000)>>12 = -0x100 (unclamped), MAC1 = (-0x100*0x800 + 0x100000)>>12 = 0x80 */
	CHECK_EQ(D(25), 0x80);
	CHECK_EQ(C(31), 0);
}

static void test_gpf_gpl(void)
{
	PC_GteReset();
	SETD(6, 0x55000000);
	SETD(9, 0x100);
	SETD(10, 0x200);
	SETD(11, -0x300);
	SETD(8, 0x800);
	gte_gpf12();
	CHECK_EQ(D(25), 0x80);
	CHECK_EQ(D(26), 0x100);
	CHECK_EQ((int32_t)D(27), -0x180);
	CHECK_EQ(D(11), 0xFFFFFE80);
	CHECK_EQ(D(22), 0x55001008);
	CHECK_EQ(C(31), 0x00080000);

	SETD(25, 0x10);
	SETD(26, 0x20);
	SETD(27, 0x30);
	SETD(9, 0x100);
	SETD(10, 0x200);
	SETD(11, -0x300);
	gte_gpl12();
	CHECK_EQ(D(25), 0x90);
	CHECK_EQ(D(26), 0x120);
	CHECK_EQ((int32_t)D(27), -0x150);
	CHECK_EQ(D(22), 0x55001209);
	CHECK_EQ(D(21), 0x55001008);

	SETD(25, 0x10);
	SETD(9, 0x100);
	gte_gpl0();
	CHECK_EQ(D(25), 0x80010);
	CHECK_EQ(D(9), 0x7FFF);
	CHECK_EQ(D(22) & 0xFF, 0xFF);
	CHECK_EQ(C(31) & 0x81200000, 0x81200000);

	/* GPF sf = 0, lm = 1 */
	SETD(9, -0x10);
	SETD(10, 0x10);
	SETD(11, 0);
	SETD(8, 0x10);
	CMD(GPF, 0, 1);
	CHECK_EQ((int32_t)D(25), -0x100);
	CHECK_EQ(D(9), 0);
	CHECK_EQ(D(10), 0x100);
	CHECK_EQ(D(22) & 0xFFFFFF, 0x1000);
}

/* the inline_c.h macros against real structures */
static void test_macros(void)
{
	MATRIX m = {{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}, {0, 0, 0}};
	MATRIX r;
	SVECTOR sv = {100, 50, 1000, 0};
	VECTOR lv = {-3, 0x12345, 7, 0};
	long sxy, otz, flag, lzc;
	SVECTOR out;
	VECTOR lout;

	PC_GteReset();
	gte_SetRotMatrix(&m);
	gte_SetTransMatrix(&m);
	gte_SetGeomOffset(160, 120);
	gte_SetGeomScreen(1000);
	gte_ldv0(&sv);
	gte_rtps();
	gte_stsxy(&sxy);
	CHECK_EQ(sxy, XY(260, 170));
	gte_stflg(&flag);
	CHECK_EQ(flag, 0);
	gte_ReadRotMatrix(&r);
	CHECK_EQ(r.m[0][0], 0x1000);
	CHECK_EQ(r.m[2][2], 0x1000);
	CHECK_EQ(r.m[0][1], 0);

	/* VECTOR -> V0 keeps the low halves */
	gte_ldlv0(&lv);
	CHECK_EQ(D(0), XY(-3, 0x2345));
	CHECK_EQ(D(1), 7);

	/* ldsv zero-extends but the IR registers read back signed */
	out.vx = -2;
	out.vy = 3;
	out.vz = -32768;
	gte_ldsv(&out);
	CHECK_EQ(D(9), 0xFFFFFFFE);
	CHECK_EQ(D(11), 0xFFFF8000);
	gte_stsv(&out);
	CHECK_EQ(out.vx, -2);
	CHECK_EQ(out.vz, -32768);

	/* outer product through ldopv1/ldopv2 and stlvnl */
	{
		VECTOR a = {0x1000, 0, 0, 0}, b = {0, 0x1000, 0, 0};
		gte_ldopv1(&a);
		gte_ldopv2(&b);
		gte_op12();
		gte_stlvnl(&lout);
		CHECK_EQ(lout.vx, 0);
		CHECK_EQ(lout.vy, 0);
		CHECK_EQ(lout.vz, 0x1000);
		gte_stlvnl2(&lout.vx);
		CHECK_EQ(lout.vx, 0x1000);
	}

	/* LZC through the macros */
	gte_ldlzc(0x00012345);
	gte_nop();
	gte_nop();
	gte_stlzc(&lzc);
	CHECK_EQ(lzc, 15);

	/* AVSZ4 and OTZ store */
	gte_ldsz4(100, 200, 300, 400);
	SETC(30, 0x400);
	gte_avsz4();
	gte_stotz(&otz);
	CHECK_EQ(otz, 250);

	/* pointer arguments are evaluated once */
	{
		SVECTOR vs[2] = {{1, 2, 3, 0}, {4, 5, 6, 0}};
		SVECTOR *p = vs;
		gte_ldv0(p++);
		CHECK_EQ(p - vs, 1);
		CHECK_EQ(D(0), XY(1, 2));
		gte_ldsv(p++);
		CHECK_EQ(p - vs, 2);
		CHECK_EQ(D(10), 5);
	}

	/* FlipRotMatrixX negates row 1 only */
	{
		MATRIX f = {{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}, {0, 0, 0}};
		gte_SetRotMatrix(&f);
		gte_FlipRotMatrixX();
		gte_ReadRotMatrix(&r);
		CHECK_EQ(r.m[0][0], -1);
		CHECK_EQ(r.m[0][1], -2);
		CHECK_EQ(r.m[0][2], -3);
		CHECK_EQ(r.m[1][0], 4);
		CHECK_EQ(r.m[1][1], 5);
		SETC(5, 10);
		gte_FlipTRX();
		CHECK_EQ((int32_t)C(5), -10);
	}

	/* packed difference / sum */
	{
		long a = (long)XY(10, 100), b = (long)XY(3, 300), dl[2];
		short dd[2];
		gte_subdvl(&a, &b, dl);
		CHECK_EQ(dl[0], 7);
		CHECK_EQ(dl[1], -200);
		gte_adddvd(&a, &b, dd);
		CHECK_EQ(dd[0], 13);
		CHECK_EQ(dd[1], 400);
	}

	/* colour FIFO via strgb, RGBC via ldrgb */
	{
		CVECTOR c = {0x80, 0x40, 0x20, 0x30}, o;
		gte_ldrgb(&c);
		gte_lddp(0);
		SETD(9, 0);
		SETD(10, 0);
		SETD(11, 0);
		gte_dpcs();
		gte_strgb(&o);
		CHECK_EQ(o.r, 0x80);
		CHECK_EQ(o.g, 0x40);
		CHECK_EQ(o.b, 0x20);
		CHECK_EQ(o.cd, 0x30);
	}
}

int main(void)
{
	test_registers();
	test_rtps();
	test_rtpt();
	test_divide();
	test_nclip_avsz();
	test_op_sqr();
	test_mvmva();
	test_colour();
	test_gpf_gpl();
	test_macros();
	printf("%d/%d checks passed\n", checks - failures, checks);
	return failures ? 1 : 0;
}

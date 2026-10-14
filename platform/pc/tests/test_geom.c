/* Checks the port's geometry maths (pc_geom.h) against the register-level GTE (src/pc_gte.c, the
 * reference the retail code is tested on): every operation the game uses, in every variant it
 * uses, on random inputs biased towards the edges (saturation, overflow, the divide's limits),
 * plus an exhaustive sweep of the perspective divide. Any difference in any result or FLAG bit
 * fails.
 *
 *   gcc -m32 -O2 -std=gnu11 -DPC_MIPS -Iinclude tests/test_geom.c src/pc_gte.c src/pc_geom.c
 *   test_geom [--count N] [--seed S]      (N cases per operation variant, default 200000) */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_geom.h"
#include "pc_gte.h"

volatile uint32_t pc_heartbeat; /* pc_gte.c */

static uint64_t s_rng = 0x9E3779B97F4A7C15ull;
static long s_fail, s_cases;
static const char *s_op;

static uint32_t rnd(void)
{
	s_rng ^= s_rng << 13;
	s_rng ^= s_rng >> 7;
	s_rng ^= s_rng << 17;
	return (uint32_t)(s_rng >> 16);
}

/* a 16-bit value: often an edge, else small or anything */
static int16_t r16(void)
{
	static const int16_t edge[] = { 0, 1, -1, 2, 0x7fff, -0x8000, 0x7ffe, -0x7fff, 0x1000, -0x1000,
		                            0x4000, -0x4000, 0x0fff, 0x00ff, 0x0100, 0x0080 };
	switch (rnd() & 3) {
	case 0: return edge[rnd() & 15];
	case 1: return (int16_t)((int32_t)(rnd() & 0x1fff) - 0x1000);
	default: return (int16_t)rnd();
	}
}

static int32_t r32(void)
{
	static const int32_t edge[] = { 0, 1, -1, 0x7fffffff, (int32_t)0x80000000, 0x7fff, -0x8000,
		                            0x10000, -0x10000, 0x00ffffff, -0x01000000, 0x01400000,
		                            0x7ff, -0x800, 0x8000000, -0x8000000 };
	switch (rnd() & 3) {
	case 0: return edge[rnd() & 15];
	case 1: return (int32_t)(rnd() & 0xffff) - 0x8000;
	case 2: return (int32_t)(rnd() & 0xffffff) - 0x800000;
	default: return (int32_t)rnd();
	}
}

static uint32_t pack(int16_t lo, int16_t hi)
{
	return (uint16_t)lo | (uint32_t)(uint16_t)hi << 16;
}

/* random control registers (the shared pc_geom), with sensible screens most of the time */
static void random_state(void)
{
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			pc_geom.rot[i][j] = r16();
			pc_geom.llm[i][j] = r16();
			pc_geom.lcm[i][j] = r16();
		}
		pc_geom.tr[i] = r32();
		pc_geom.bk[i] = r32();
		pc_geom.fc[i] = (rnd() & 1) ? (int32_t)(rnd() & 0xff) << 4 : r32();
	}
	if (rnd() & 1) {
		pc_geom.ofx = (int32_t)(rnd() & 0x1ff) << 16;
		pc_geom.ofy = (int32_t)(rnd() & 0xff) << 16;
		pc_geom.h = (uint16_t)(rnd() & 0x3ff);
	} else {
		pc_geom.ofx = r32();
		pc_geom.ofy = r32();
		pc_geom.h = (uint16_t)r16();
	}
	pc_geom.dqa = r16();
	pc_geom.dqb = r32();
	pc_geom.zsf3 = r16();
	pc_geom.zsf4 = (rnd() & 1) ? 256 : r16();
}

/* random data registers */
static void random_data(void)
{
	int r;
	for (r = 0; r < 31; r++)
		if (r != 15 && r != 28 && r != 29 && r != 30)
			PC_GteSetData(r, (rnd() & 1) ? pack(r16(), r16()) : (uint32_t)r32());
	PC_GteSetData(30, (uint32_t)r32());
	PC_GteSetCtrl(31, 0);
}

static void check(const char *what, uint32_t got, uint32_t want)
{
	if (got != want) {
		if (s_fail < 40)
			printf("%s: %s = %08x, reference %08x (seed state %016llx)\n", s_op, what, got, want,
			       (unsigned long long)s_rng);
		s_fail++;
	}
}

static int16_t ir(int i)
{
	return (int16_t)PC_GteGetData(8 + i);
}

static void check_acc(const GeomAcc *a)
{
	int i;
	for (i = 0; i < 3; i++) {
		check("mac", (uint32_t)a->mac[i], PC_GteGetData(25 + i));
		check("ir", (uint32_t)(int32_t)a->ir[i], PC_GteGetData(9 + i));
	}
}

static uint32_t flagreg(void)
{
	return PC_GteGetCtrl(31);
}

/* ---- projection observer: both implementations must report the same vertices ---- */

typedef struct {
	int64_t acc[3];
	int shift, clamped;
	uint32_t sxy, flag;
} ObsRec;
static ObsRec s_obs[2][3];
static int s_nobs[2], s_side;

static void observer(const GeomState *g, const int64_t acc[3], int shift, uint32_t sxy,
                     uint32_t flag, int clamped)
{
	ObsRec *r;
	(void)g;
	if (s_nobs[s_side] >= 3)
		return;
	r = &s_obs[s_side][s_nobs[s_side]++];
	memcpy(r->acc, acc, sizeof r->acc);
	r->shift = shift;
	r->sxy = sxy;
	r->flag = flag;
	r->clamped = clamped;
}

static void check_observed(void)
{
	int i;
	check("observed vertices", (uint32_t)s_nobs[1], (uint32_t)s_nobs[0]);
	for (i = 0; i < s_nobs[0] && i < s_nobs[1]; i++) {
		check("observed acc", memcmp(s_obs[1][i].acc, s_obs[0][i].acc, 24) ? 1 : 0, 0);
		check("observed sxy", s_obs[1][i].sxy, s_obs[0][i].sxy);
		check("observed flag", s_obs[1][i].flag, s_obs[0][i].flag);
		check("observed clamp", (uint32_t)s_obs[1][i].clamped, (uint32_t)s_obs[0][i].clamped);
	}
	s_nobs[0] = s_nobs[1] = 0;
}

/* ---- operations ---- */

static void test_rtps(int sf, int lm)
{
	GeomProj p;
	GeomDepthQ dq;
	uint32_t f;
	int16_t v[3];
	GeomState st;

	random_state();
	random_data();
	v[0] = (int16_t)PC_GteGetData(0);
	v[1] = (int16_t)(PC_GteGetData(0) >> 16);
	v[2] = (int16_t)PC_GteGetData(1);
	st = pc_geom;
	s_side = 0;
	PC_GteCommand(0x0000001 | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	s_side = 1;
	p = geom_rtps(&st, v[0], v[1], v[2], sf * 12, lm, &dq, &f);
	check_acc(&p.v);
	check("sxy", p.sxy, PC_GteGetData(14));
	check("sz", p.sz, PC_GteGetData(19));
	check("mac0", (uint32_t)dq.mac0, PC_GteGetData(24));
	check("ir0", (uint32_t)(int32_t)dq.ir0, PC_GteGetData(8));
	check("flag", f, flagreg());
	check_observed();
	/* the cheap form gives the same projection */
	p = geom_rtps(&st, v[0], v[1], v[2], sf * 12, lm, NULL, NULL);
	check("sxy (no flag)", p.sxy, PC_GteGetData(14));
	s_nobs[1] = 0;
}

static void test_rtpt(int sf, int lm)
{
	GeomProj p[3];
	GeomDepthQ dq;
	uint32_t f;
	int16_t v[3][3];
	int i;
	GeomState st;

	random_state();
	random_data();
	for (i = 0; i < 3; i++) {
		v[i][0] = (int16_t)PC_GteGetData(2 * i);
		v[i][1] = (int16_t)(PC_GteGetData(2 * i) >> 16);
		v[i][2] = (int16_t)PC_GteGetData(2 * i + 1);
	}
	st = pc_geom;
	s_side = 0;
	PC_GteCommand(0x0000030 | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	s_side = 1;
	geom_rtpt(&st, v, sf * 12, lm, p, &dq, &f);
	check_acc(&p[2].v);
	for (i = 0; i < 3; i++) {
		check("sxy", p[i].sxy, PC_GteGetData(12 + i));
		check("sz", p[i].sz, PC_GteGetData(17 + i));
	}
	check("mac0", (uint32_t)dq.mac0, PC_GteGetData(24));
	check("ir0", (uint32_t)(int32_t)dq.ir0, PC_GteGetData(8));
	check("flag", f, flagreg());
	check_observed();
}

static void test_mvmva(int sf, int mx, int vi, int cv, int lm)
{
	GeomAcc a;
	uint32_t f;
	int32_t v[3];
	const int16_t(*m)[3];
	const int32_t *t;
	GeomState st;

	random_state();
	random_data();
	if (vi == 3) {
		v[0] = ir(1);
		v[1] = ir(2);
		v[2] = ir(3);
	} else {
		v[0] = (int16_t)PC_GteGetData(2 * vi);
		v[1] = (int16_t)(PC_GteGetData(2 * vi) >> 16);
		v[2] = (int16_t)PC_GteGetData(2 * vi + 1);
	}
	st = pc_geom;
	m = mx == 0 ? st.rot : mx == 1 ? st.llm : st.lcm;
	t = cv == 0 ? st.tr : cv == 1 ? st.bk : NULL;
	PC_GteCommand(0x0400012 | (uint32_t)sf << 19 | (uint32_t)mx << 17 | (uint32_t)vi << 15 |
	              (uint32_t)cv << 13 | (uint32_t)lm << 10);
	a = geom_mvmva(m, t, v[0], v[1], v[2], sf * 12, lm, &f);
	check_acc(&a);
	check("flag", f, flagreg());
}

static void test_op(int sf, int lm)
{
	GeomAcc a;
	uint32_t f;
	int16_t d[3], v[3];

	random_state();
	random_data();
	geom_op_diag(&pc_geom, d);
	v[0] = ir(1);
	v[1] = ir(2);
	v[2] = ir(3);
	PC_GteCommand(0x000000c | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	a = geom_op(d, v, sf * 12, lm, &f);
	check_acc(&a);
	check("flag", f, flagreg());
}

static void test_sqr(int sf, int lm)
{
	GeomAcc a;
	uint32_t f;
	int16_t v[3];

	random_data();
	v[0] = ir(1);
	v[1] = ir(2);
	v[2] = ir(3);
	PC_GteCommand(0x0000028 | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	a = geom_sqr(v, sf * 12, lm, &f);
	check_acc(&a);
	check("flag", f, flagreg());
}

static void test_gpf_gpl(int gpl, int sf, int lm)
{
	GeomAcc a;
	uint32_t f, rgb;
	int16_t v[3], k;
	int32_t base[3];
	uint8_t code;
	int i;

	random_data();
	k = ir(0);
	for (i = 0; i < 3; i++) {
		v[i] = ir(i + 1);
		base[i] = (int32_t)PC_GteGetData(25 + i);
	}
	code = (uint8_t)(PC_GteGetData(6) >> 24);
	PC_GteCommand((gpl ? 0x000003e : 0x000003d) | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	if (gpl)
		a = geom_gpl(k, v, base, sf * 12, lm, code, &rgb, &f);
	else
		a = geom_gpf(k, v, sf * 12, lm, code, &rgb, &f);
	check_acc(&a);
	check("rgb", rgb, PC_GteGetData(22));
	check("flag", f, flagreg());
}

static void test_dpcs(int sf, int lm)
{
	GeomAcc a;
	uint32_t f, rgb, col;
	int16_t k;
	GeomState st;

	random_state();
	random_data();
	col = PC_GteGetData(6);
	k = ir(0);
	st = pc_geom;
	PC_GteCommand(0x0000010 | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	a = geom_dpcs(&st, col & 0xffffff, (uint8_t)(col >> 24), k, sf * 12, lm, &rgb, &f);
	check_acc(&a);
	check("rgb", rgb, PC_GteGetData(22));
	check("flag", f, flagreg());
}

static void test_dpct(int sf, int lm)
{
	uint32_t f, rgb[3], col[3];
	uint8_t code;
	int16_t k;
	int i;
	GeomState st;

	random_state();
	random_data();
	for (i = 0; i < 3; i++)
		col[i] = PC_GteGetData(20 + i);
	code = (uint8_t)(PC_GteGetData(6) >> 24);
	k = ir(0);
	st = pc_geom;
	PC_GteCommand(0x000002a | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	/* the hardware runs DPCS on RGB0 three times, the FIFO moving under it */
	{
		uint32_t in[3] = { col[0], col[1], col[2] };
		geom_dpct(&st, in, code, k, sf * 12, lm, rgb, &f);
	}
	for (i = 0; i < 3; i++)
		check("rgb", rgb[i], PC_GteGetData(20 + i));
	check("flag", f, flagreg());
}

static void test_dpcl(int sf, int lm)
{
	GeomAcc a;
	uint32_t f, rgb, col;
	int16_t k, v[3];
	GeomState st;

	random_state();
	random_data();
	col = PC_GteGetData(6);
	k = ir(0);
	v[0] = ir(1);
	v[1] = ir(2);
	v[2] = ir(3);
	st = pc_geom;
	PC_GteCommand(0x0000029 | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	a = geom_dpcl(&st, col, v, k, sf * 12, lm, &rgb, &f);
	check_acc(&a);
	check("rgb", rgb, PC_GteGetData(22));
	check("flag", f, flagreg());
}

static void test_nccs(int sf, int lm)
{
	GeomAcc a;
	uint32_t f, rgb, col;
	int16_t v[3];
	GeomState st;

	random_state();
	random_data();
	col = PC_GteGetData(6);
	v[0] = (int16_t)PC_GteGetData(0);
	v[1] = (int16_t)(PC_GteGetData(0) >> 16);
	v[2] = (int16_t)PC_GteGetData(1);
	st = pc_geom;
	PC_GteCommand(0x000001b | (uint32_t)sf << 19 | (uint32_t)lm << 10);
	a = geom_nccs(&st, v[0], v[1], v[2], col, sf * 12, lm, &rgb, &f);
	check_acc(&a);
	check("rgb", rgb, PC_GteGetData(22));
	check("flag", f, flagreg());
}

static void test_nclip(void)
{
	uint32_t f, s0, s1, s2;
	int32_t m;

	random_data();
	s0 = PC_GteGetData(12);
	s1 = PC_GteGetData(13);
	s2 = PC_GteGetData(14);
	PC_GteCommand(0x1400006);
	m = geom_nclip(s0, s1, s2, &f);
	check("mac0", (uint32_t)m, PC_GteGetData(24));
	check("flag", f, flagreg());
	check("mac0 (no flag)", (uint32_t)geom_nclip(s0, s1, s2, NULL), PC_GteGetData(24));
}

static void test_avsz4(void)
{
	uint32_t f;
	uint16_t sz[4], otz;
	int32_t m;
	int i;
	GeomState st;

	random_state();
	random_data();
	for (i = 0; i < 4; i++)
		sz[i] = (uint16_t)PC_GteGetData(16 + i);
	st = pc_geom;
	PC_GteCommand(0x168002e);
	otz = geom_avsz4(&st, sz[0], sz[1], sz[2], sz[3], &m, &f);
	check("otz", otz, PC_GteGetData(7));
	check("mac0", (uint32_t)m, PC_GteGetData(24));
	check("flag", f, flagreg());
}

static void test_lzc(void)
{
	int32_t v = r32();
	if (rnd() & 1)
		v = (int32_t)(rnd() >> (rnd() & 31)) * ((rnd() & 1) ? -1 : 1);
	PC_GteSetData(30, (uint32_t)v);
	check("lzcr", (uint32_t)geom_lzc(v), PC_GteGetData(31));
}

/* every depth through the divide, for a spread of screen distances */
static void sweep_divide(void)
{
	static const uint16_t hs[] = { 0, 1, 2, 100, 160, 200, 256, 300, 320, 400, 512, 640, 1000,
		                           0x3ff, 0x7fff, 0x8000, 0xfffe, 0xffff };
	unsigned i, z;
	s_op = "divide sweep";
	memset(&pc_geom, 0, sizeof pc_geom);
	pc_geom.ofx = 160 << 16;
	pc_geom.ofy = 120 << 16;
	for (i = 0; i < sizeof hs / sizeof hs[0]; i++) {
		pc_geom.h = hs[i];
		for (z = 0; z <= 0xffff; z++) {
			GeomProj p;
			uint32_t f;
			pc_geom.tr[0] = 0x7f;
			pc_geom.tr[1] = -0x55;
			pc_geom.tr[2] = (int32_t)z;
			PC_GteSetData(0, 0);
			PC_GteSetData(1, 0);
			PC_GteCommand(0x0080001);
			p = geom_rtps(&pc_geom, 0, 0, 0, 12, 0, NULL, &f);
			check("sxy", p.sxy, PC_GteGetData(14));
			check("flag", f, flagreg());
			s_cases++;
		}
	}
}

static void sweep_table(void)
{
	int i;
	s_op = "unr table";
	for (i = 0; i <= 0x100; i++) {
		int v = (0x40000 / (i + 0x100) + 1) / 2 - 0x101;
		check("entry", geom_unr_table[i], (uint32_t)(v < 0 ? 0 : v));
	}
}

int main(int argc, char **argv)
{
	long count = 200000, n;
	int i, sf, lm, mx, vi, cv;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--count") && i + 1 < argc)
			count = atol(argv[++i]);
		else if (!strcmp(argv[i], "--seed") && i + 1 < argc)
			s_rng = strtoull(argv[++i], NULL, 0) | 1;
	}
	PC_GteReset();
	sweep_table();
	sweep_divide();
	pc_geom_observer = observer;

#define RUN(name, call)                                                                            \
	do {                                                                                           \
		s_op = name;                                                                               \
		for (n = 0; n < count; n++)                                                                \
			call;                                                                                  \
		s_cases += count;                                                                          \
	} while (0)

	for (sf = 0; sf < 2; sf++) {
		for (lm = 0; lm < 2; lm++) {
			RUN("rtps", test_rtps(sf, lm));
			RUN("rtpt", test_rtpt(sf, lm));
			for (mx = 0; mx < 3; mx++)
				for (vi = 0; vi < 4; vi++)
					for (cv = 0; cv < 4; cv++)
						if (cv != 2)
							RUN("mvmva", test_mvmva(sf, mx, vi, cv, lm));
			RUN("op", test_op(sf, lm));
			RUN("sqr", test_sqr(sf, lm));
			RUN("gpf", test_gpf_gpl(0, sf, lm));
			RUN("gpl", test_gpf_gpl(1, sf, lm));
			RUN("dpcs", test_dpcs(sf, lm));
			RUN("dpct", test_dpct(sf, lm));
			RUN("dpcl", test_dpcl(sf, lm));
			RUN("nccs", test_nccs(sf, lm));
		}
	}
	RUN("nclip", test_nclip());
	RUN("avsz4", test_avsz4());
	RUN("lzc", test_lzc());

	printf("%ld cases, %ld differences\n", s_cases, s_fail);
	return s_fail ? 1 : 0;
}

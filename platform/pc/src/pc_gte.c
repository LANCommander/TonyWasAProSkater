/* Software GTE (PS1 COP2), register-accurate, following psx-spx "Geometry Transformation Engine".
 * MAC1-3 accumulate in 44 bits with an overflow check (and wrap) after every addition; MAC0 is
 * checked against 32 bits. Division is the hardware's UNR (Newton-Raphson) reciprocal. */
#include "pc_gte.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern volatile uint32_t pc_heartbeat;
static int nclip_dump_active(void)
{
	static int from = -2, to = -2;
	if (from == -2) {
		const char *e = getenv("THPS_GPUDUMP");
		if (!e || sscanf(e, "%d-%d", &from, &to) != 2)
			from = to = -1;
	}
	return from >= 0 && (int)pc_heartbeat >= from && (int)pc_heartbeat <= to;
}

typedef struct {
	/* data registers */
	int16_t v[3][3]; /* V0..V2 (x, y, z) */
	uint8_t rgbc[4]; /* R, G, B, CODE */
	uint16_t otz;
	int16_t ir[4];
	int16_t sx[3], sy[3]; /* SXY0..SXY2 */
	uint16_t sz[4];       /* SZ0..SZ3 */
	uint32_t rgb[3];      /* colour FIFO RGB0..RGB2 */
	uint32_t res1;
	int32_t mac[4];
	uint32_t lzcs, lzcr;
	/* control registers */
	int16_t rt[3][3];
	int32_t tr[3];
	int16_t llm[3][3];
	int32_t bk[3];
	int16_t lcm[3][3];
	int32_t fc[3];
	int32_t ofx, ofy;
	uint16_t h;
	int16_t dqa;
	int32_t dqb;
	int16_t zsf3, zsf4;
	uint32_t flag;
} Gte;

static Gte g;

enum {
	F_MAC1_POS = 1u << 30,
	F_MAC1_NEG = 1u << 27,
	F_IR0 = 1u << 12,
	F_SY2 = 1u << 13,
	F_SX2 = 1u << 14,
	F_MAC0_NEG = 1u << 15,
	F_MAC0_POS = 1u << 16,
	F_DIV = 1u << 17,
	F_SZ3_OTZ = 1u << 18,
	F_ERROR_MASK = 0x7F87E000u,
};

/* ---- UNR division ---------------------------------------------------------------------------- */

static uint8_t unr_table[257];
static int unr_ready;

static void build_unr_table(void)
{
	for (int i = 0; i <= 0x100; i++) {
		int v = (0x40000 / (i + 0x100) + 1) / 2 - 0x101;
		unr_table[i] = (uint8_t)(v < 0 ? 0 : v);
	}
	unr_ready = 1;
}

static uint32_t gte_divide(uint32_t h, uint32_t sz3)
{
	if (h >= sz3 * 2) {
		g.flag |= F_DIV;
		return 0x1FFFF;
	}
	if (!unr_ready)
		build_unr_table();
	int z = 0;
	while (z < 16 && !(sz3 & (0x8000u >> z)))
		z++;
	uint32_t n = h << z;
	uint32_t d = sz3 << z;
	uint32_t u = unr_table[(d - 0x7FC0) >> 7] + 0x101;
	d = (0x2000080 - d * u) >> 8;
	d = (0x0000080 + d * u) >> 8;
	uint64_t r = ((uint64_t)n * d + 0x8000) >> 16;
	return r > 0x1FFFF ? 0x1FFFF : (uint32_t)r;
}

/* ---- saturation and flags -------------------------------------------------------------------- */

static inline int64_t sext44(int64_t v)
{
	return (int64_t)((uint64_t)v << 20) >> 20;
}

static inline void mac_check(int i, int64_t v)
{
	if (v > 0x7FFFFFFFFFFLL)
		g.flag |= F_MAC1_POS >> (i - 1);
	else if (v < -0x80000000000LL)
		g.flag |= F_MAC1_NEG >> (i - 1);
}

/* One accumulation step of MAC1-3: check, then wrap to 44 bits. */
static inline int64_t mac_step(int i, int64_t v)
{
	mac_check(i, v);
	return sext44(v);
}

static inline void mac0_check(int64_t v)
{
	if (v > 0x7FFFFFFFLL)
		g.flag |= F_MAC0_POS;
	else if (v < -0x80000000LL)
		g.flag |= F_MAC0_NEG;
}

static inline void set_ir(int i, int32_t v, int lm)
{
	int32_t lo = lm ? 0 : -0x8000;
	if (v < lo) {
		v = lo;
		g.flag |= 1u << (25 - i);
	} else if (v > 0x7FFF) {
		v = 0x7FFF;
		g.flag |= 1u << (25 - i);
	}
	g.ir[i] = (int16_t)v;
}

static inline void set_mac_ir(int i, int64_t v, int shift, int lm)
{
	mac_check(i, v);
	int32_t m = (int32_t)(v >> shift);
	g.mac[i] = m;
	set_ir(i, m, lm);
}

static void push_sz(int64_t z)
{
	int64_t raw = z;
	if (z < 0) {
		z = 0;
		g.flag |= F_SZ3_OTZ;
	} else if (z > 0xFFFF) {
		z = 0xFFFF;
		g.flag |= F_SZ3_OTZ;
	}
	if (nclip_dump_active() && raw != z)
		fprintf(stderr, "push_sz f%u raw=%lld clamped_to=%lld\n", pc_heartbeat, (long long)raw,
		        (long long)z);
	g.sz[0] = g.sz[1];
	g.sz[1] = g.sz[2];
	g.sz[2] = g.sz[3];
	g.sz[3] = (uint16_t)z;
}

/* THPS_GPUDUMP diagnostic only: sticky per-vertex saturation, parallel to the sx/sy ring, so
 * cmd_nclip can tell whether any of the 3 vertices it's testing got clamped (g.flag is reset at
 * the top of every GTE op, so it can't see a prior RTPS's saturation by the time NCLIP runs). */
static int s_sat_ring[3];

static void push_sxy(int64_t x, int64_t y)
{
	int sat = 0;
	if (x < -0x400) {
		x = -0x400;
		g.flag |= F_SX2;
		sat = 1;
	} else if (x > 0x3FF) {
		x = 0x3FF;
		g.flag |= F_SX2;
		sat = 1;
	}
	if (y < -0x400) {
		y = -0x400;
		g.flag |= F_SY2;
		sat = 1;
	} else if (y > 0x3FF) {
		y = 0x3FF;
		g.flag |= F_SY2;
		sat = 1;
	}
	s_sat_ring[0] = s_sat_ring[1];
	s_sat_ring[1] = s_sat_ring[2];
	s_sat_ring[2] = sat;
	g.sx[0] = g.sx[1];
	g.sy[0] = g.sy[1];
	g.sx[1] = g.sx[2];
	g.sy[1] = g.sy[2];
	g.sx[2] = (int16_t)x;
	g.sy[2] = (int16_t)y;
}

static inline uint32_t clamp_col(int32_t v, uint32_t flag)
{
	if (v < 0) {
		g.flag |= flag;
		return 0;
	}
	if (v > 0xFF) {
		g.flag |= flag;
		return 0xFF;
	}
	return (uint32_t)v;
}

static void push_rgb_from_mac(void)
{
	uint32_t r = clamp_col(g.mac[1] >> 4, 1u << 21);
	uint32_t gg = clamp_col(g.mac[2] >> 4, 1u << 20);
	uint32_t b = clamp_col(g.mac[3] >> 4, 1u << 19);
	g.rgb[0] = g.rgb[1];
	g.rgb[1] = g.rgb[2];
	g.rgb[2] = r | (gg << 8) | (b << 16) | ((uint32_t)g.rgbc[3] << 24);
}

static void set_otz(int64_t v)
{
	if (v < 0) {
		v = 0;
		g.flag |= F_SZ3_OTZ;
	} else if (v > 0xFFFF) {
		v = 0xFFFF;
		g.flag |= F_SZ3_OTZ;
	}
	g.otz = (uint16_t)v;
}

/* ---- shared arithmetic ----------------------------------------------------------------------- */

/* MAC/IR = (T*1000h + M*V) SAR shift; T may be NULL (zero). */
static void mul_mat_vec(int16_t m[3][3], const int32_t *t, int32_t vx, int32_t vy, int32_t vz,
                        int shift, int lm)
{
	for (int i = 0; i < 3; i++) {
		int64_t acc = t ? (int64_t)t[i] * 4096 : 0;
		acc = mac_step(i + 1, acc + (int64_t)m[i][0] * vx);
		acc = mac_step(i + 1, acc + (int64_t)m[i][1] * vy);
		acc = acc + (int64_t)m[i][2] * vz;
		set_mac_ir(i + 1, acc, shift, lm);
	}
}

/* MAC = in + (FC*1000h - in) * IR0, then SAR shift. The intermediate IR saturates signed. */
static void depth_cue(int64_t m1, int64_t m2, int64_t m3, int shift, int lm)
{
	set_mac_ir(1, (int64_t)g.fc[0] * 4096 - m1, shift, 0);
	set_mac_ir(2, (int64_t)g.fc[1] * 4096 - m2, shift, 0);
	set_mac_ir(3, (int64_t)g.fc[2] * 4096 - m3, shift, 0);
	set_mac_ir(1, (int64_t)g.ir[1] * g.ir[0] + m1, shift, lm);
	set_mac_ir(2, (int64_t)g.ir[2] * g.ir[0] + m2, shift, lm);
	set_mac_ir(3, (int64_t)g.ir[3] * g.ir[0] + m3, shift, lm);
}

static void color_ir_products(int64_t *m)
{
	m[0] = ((int64_t)g.rgbc[0] * g.ir[1]) * 16;
	m[1] = ((int64_t)g.rgbc[1] * g.ir[2]) * 16;
	m[2] = ((int64_t)g.rgbc[2] * g.ir[3]) * 16;
}

/* ---- commands -------------------------------------------------------------------------------- */

static void cmd_rtp(int idx, int shift, int lm, int last)
{
	int32_t vx = g.v[idx][0], vy = g.v[idx][1], vz = g.v[idx][2];
	int64_t acc[3];
	for (int i = 0; i < 3; i++) {
		int64_t a = (int64_t)g.tr[i] * 4096;
		a = mac_step(i + 1, a + (int64_t)g.rt[i][0] * vx);
		a = mac_step(i + 1, a + (int64_t)g.rt[i][1] * vy);
		acc[i] = a + (int64_t)g.rt[i][2] * vz;
	}
	set_mac_ir(1, acc[0], shift, lm);
	set_mac_ir(2, acc[1], shift, lm);

	/* IR3 saturates on MAC3, but its flag reflects MAC3 SAR 12 regardless of sf. */
	mac_check(3, acc[2]);
	g.mac[3] = (int32_t)(acc[2] >> shift);
	int64_t z12 = acc[2] >> 12;
	if (z12 < -0x8000 || z12 > 0x7FFF)
		g.flag |= 1u << 22;
	int32_t ir3 = g.mac[3];
	int32_t lo = lm ? 0 : -0x8000;
	g.ir[3] = (int16_t)(ir3 < lo ? lo : ir3 > 0x7FFF ? 0x7FFF : ir3);

	push_sz(z12);
	uint32_t n = gte_divide(g.h, g.sz[3]);

	int64_t x = (int64_t)n * g.ir[1] + g.ofx;
	int64_t y = (int64_t)n * g.ir[2] + g.ofy;
	mac0_check(x);
	mac0_check(y);
	push_sxy(x >> 16, y >> 16);

	if (last) {
		int64_t dq = (int64_t)n * g.dqa + g.dqb;
		mac0_check(dq);
		g.mac[0] = (int32_t)dq;
		int64_t i0 = dq >> 12;
		if (i0 < 0) {
			i0 = 0;
			g.flag |= F_IR0;
		} else if (i0 > 0x1000) {
			i0 = 0x1000;
			g.flag |= F_IR0;
		}
		g.ir[0] = (int16_t)i0;
	}
}

static void cmd_nclip(void)
{
	int64_t v = (int64_t)g.sx[0] * g.sy[1] + (int64_t)g.sx[1] * g.sy[2] + (int64_t)g.sx[2] * g.sy[0] -
	            (int64_t)g.sx[0] * g.sy[2] - (int64_t)g.sx[1] * g.sy[0] - (int64_t)g.sx[2] * g.sy[1];
	mac0_check(v);
	g.mac[0] = (int32_t)v;
	if (nclip_dump_active()) {
		int sat = s_sat_ring[0] || s_sat_ring[1] || s_sat_ring[2];
		fprintf(stderr, "nclip f%u (%d,%d)-(%d,%d)-(%d,%d) area=%lld%s\n", pc_heartbeat, g.sx[0],
		        g.sy[0], g.sx[1], g.sy[1], g.sx[2], g.sy[2], (long long)v,
		        sat ? " SATURATED" : "");
	}
}

static void cmd_op(int shift, int lm)
{
	int64_t d1 = g.rt[0][0], d2 = g.rt[1][1], d3 = g.rt[2][2];
	int64_t i1 = g.ir[1], i2 = g.ir[2], i3 = g.ir[3];
	set_mac_ir(1, d2 * i3 - d3 * i2, shift, lm);
	set_mac_ir(2, d3 * i1 - d1 * i3, shift, lm);
	set_mac_ir(3, d1 * i2 - d2 * i1, shift, lm);
}

static void cmd_sqr(int shift, int lm)
{
	int64_t i1 = g.ir[1], i2 = g.ir[2], i3 = g.ir[3];
	set_mac_ir(1, i1 * i1, shift, lm);
	set_mac_ir(2, i2 * i2, shift, lm);
	set_mac_ir(3, i3 * i3, shift, lm);
}

static void cmd_avsz3(void)
{
	int64_t v = (int64_t)g.zsf3 * ((int32_t)g.sz[1] + g.sz[2] + g.sz[3]);
	mac0_check(v);
	g.mac[0] = (int32_t)v;
	set_otz(v >> 12);
	if (nclip_dump_active())
		fprintf(stderr, "avsz3 f%u sz=%u,%u,%u zsf3=%d raw=%lld otz=%u%s\n", pc_heartbeat, g.sz[1],
		        g.sz[2], g.sz[3], g.zsf3, (long long)(v >> 12), g.otz,
		        (g.flag & F_SZ3_OTZ) ? " SAT" : "");
}

static void cmd_avsz4(void)
{
	int64_t v = (int64_t)g.zsf4 * ((int32_t)g.sz[0] + g.sz[1] + g.sz[2] + g.sz[3]);
	mac0_check(v);
	g.mac[0] = (int32_t)v;
	set_otz(v >> 12);
	if (nclip_dump_active())
		fprintf(stderr, "avsz4 f%u sz=%u,%u,%u,%u zsf4=%d raw=%lld otz=%u%s\n", pc_heartbeat, g.sz[0],
		        g.sz[1], g.sz[2], g.sz[3], g.zsf4, (long long)(v >> 12), g.otz,
		        (g.flag & F_SZ3_OTZ) ? " SAT" : "");
}

static void cmd_mvmva(uint32_t op, int shift, int lm)
{
	int mx = (op >> 17) & 3, vi = (op >> 15) & 3, cv = (op >> 13) & 3;
	int16_t garbage[3][3];
	int16_t(*m)[3];
	switch (mx) {
	case 0: m = g.rt; break;
	case 1: m = g.llm; break;
	case 2: m = g.lcm; break;
	default:
		garbage[0][0] = (int16_t)-(g.rgbc[0] << 4);
		garbage[0][1] = (int16_t)(g.rgbc[0] << 4);
		garbage[0][2] = g.ir[0];
		garbage[1][0] = garbage[1][1] = garbage[1][2] = g.rt[0][2];
		garbage[2][0] = garbage[2][1] = garbage[2][2] = g.rt[1][1];
		m = garbage;
		break;
	}
	int32_t vx, vy, vz;
	if (vi == 3) {
		vx = g.ir[1];
		vy = g.ir[2];
		vz = g.ir[3];
	} else {
		vx = g.v[vi][0];
		vy = g.v[vi][1];
		vz = g.v[vi][2];
	}
	if (cv == 2) {
		/* Hardware bug: FC + first column only reaches the flags; the result is the other two
		 * columns. */
		for (int i = 0; i < 3; i++) {
			int64_t t = mac_step(i + 1, (int64_t)g.fc[i] * 4096 + (int64_t)m[i][0] * vx);
			set_ir(i + 1, (int32_t)(t >> shift), 0);
			int64_t acc = mac_step(i + 1, (int64_t)m[i][1] * vy);
			acc += (int64_t)m[i][2] * vz;
			set_mac_ir(i + 1, acc, shift, lm);
		}
		return;
	}
	const int32_t *t = cv == 0 ? g.tr : cv == 1 ? g.bk : NULL;
	mul_mat_vec(m, t, vx, vy, vz, shift, lm);
}

/* NCS / NCCS / NCDS on vector idx. mode: 0 = NCS, 1 = NCCS, 2 = NCDS */
static void cmd_nc(int idx, int mode, int shift, int lm)
{
	mul_mat_vec(g.llm, NULL, g.v[idx][0], g.v[idx][1], g.v[idx][2], shift, lm);
	mul_mat_vec(g.lcm, g.bk, g.ir[1], g.ir[2], g.ir[3], shift, lm);
	if (mode) {
		int64_t m[3];
		color_ir_products(m);
		if (mode == 2) {
			depth_cue(m[0], m[1], m[2], shift, lm);
		} else {
			set_mac_ir(1, m[0], shift, lm);
			set_mac_ir(2, m[1], shift, lm);
			set_mac_ir(3, m[2], shift, lm);
		}
	}
	push_rgb_from_mac();
}

static void cmd_cdp(int shift, int lm)
{
	mul_mat_vec(g.lcm, g.bk, g.ir[1], g.ir[2], g.ir[3], shift, lm);
	int64_t m[3];
	color_ir_products(m);
	depth_cue(m[0], m[1], m[2], shift, lm);
	push_rgb_from_mac();
}

static void cmd_cc(int shift, int lm)
{
	mul_mat_vec(g.lcm, g.bk, g.ir[1], g.ir[2], g.ir[3], shift, lm);
	int64_t m[3];
	color_ir_products(m);
	set_mac_ir(1, m[0], shift, lm);
	set_mac_ir(2, m[1], shift, lm);
	set_mac_ir(3, m[2], shift, lm);
	push_rgb_from_mac();
}

static void cmd_dcpl(int shift, int lm)
{
	int64_t m[3];
	color_ir_products(m);
	depth_cue(m[0], m[1], m[2], shift, lm);
	push_rgb_from_mac();
}

static void cmd_dpc(uint32_t rgb, int shift, int lm)
{
	depth_cue((int64_t)(rgb & 0xFF) << 16, (int64_t)((rgb >> 8) & 0xFF) << 16,
	          (int64_t)((rgb >> 16) & 0xFF) << 16, shift, lm);
	push_rgb_from_mac();
}

static void cmd_intpl(int shift, int lm)
{
	depth_cue((int64_t)g.ir[1] * 4096, (int64_t)g.ir[2] * 4096, (int64_t)g.ir[3] * 4096, shift, lm);
	push_rgb_from_mac();
}

static void cmd_gpf(int shift, int lm)
{
	int64_t i0 = g.ir[0];
	set_mac_ir(1, g.ir[1] * i0, shift, lm);
	set_mac_ir(2, g.ir[2] * i0, shift, lm);
	set_mac_ir(3, g.ir[3] * i0, shift, lm);
	push_rgb_from_mac();
}

static void cmd_gpl(int shift, int lm)
{
	int64_t i0 = g.ir[0];
	int64_t m1 = (int64_t)g.mac[1] * ((int64_t)1 << shift);
	int64_t m2 = (int64_t)g.mac[2] * ((int64_t)1 << shift);
	int64_t m3 = (int64_t)g.mac[3] * ((int64_t)1 << shift);
	set_mac_ir(1, g.ir[1] * i0 + m1, shift, lm);
	set_mac_ir(2, g.ir[2] * i0 + m2, shift, lm);
	set_mac_ir(3, g.ir[3] * i0 + m3, shift, lm);
	push_rgb_from_mac();
}

void PC_GteCommand(uint32_t op)
{
	int shift = (op & (1u << 19)) ? 12 : 0;
	int lm = (op >> 10) & 1;
	g.flag = 0;
	switch (op & 0x3F) {
	case 0x01: cmd_rtp(0, shift, lm, 1); break;
	case 0x06: cmd_nclip(); break;
	case 0x0C: cmd_op(shift, lm); break;
	case 0x10: cmd_dpc(g.rgbc[0] | (g.rgbc[1] << 8) | ((uint32_t)g.rgbc[2] << 16), shift, lm); break;
	case 0x11: cmd_intpl(shift, lm); break;
	case 0x12: cmd_mvmva(op, shift, lm); break;
	case 0x13: cmd_nc(0, 2, shift, lm); break;
	case 0x14: cmd_cdp(shift, lm); break;
	case 0x16:
		for (int i = 0; i < 3; i++)
			cmd_nc(i, 2, shift, lm);
		break;
	case 0x1B: cmd_nc(0, 1, shift, lm); break;
	case 0x1C: cmd_cc(shift, lm); break;
	case 0x1E: cmd_nc(0, 0, shift, lm); break;
	case 0x20:
		for (int i = 0; i < 3; i++)
			cmd_nc(i, 0, shift, lm);
		break;
	case 0x28: cmd_sqr(shift, lm); break;
	case 0x29: cmd_dcpl(shift, lm); break;
	case 0x2A:
		for (int i = 0; i < 3; i++)
			cmd_dpc(g.rgb[0], shift, lm);
		break;
	case 0x2D: cmd_avsz3(); break;
	case 0x2E: cmd_avsz4(); break;
	case 0x30:
		cmd_rtp(0, shift, lm, 0);
		cmd_rtp(1, shift, lm, 0);
		cmd_rtp(2, shift, lm, 1);
		break;
	case 0x3D: cmd_gpf(shift, lm); break;
	case 0x3E: cmd_gpl(shift, lm); break;
	case 0x3F:
		for (int i = 0; i < 3; i++)
			cmd_nc(i, 1, shift, lm);
		break;
	default: break;
	}
	if (g.flag & F_ERROR_MASK)
		g.flag |= 0x80000000u;
}

/* ---- register access ------------------------------------------------------------------------- */

static inline uint32_t pack16(int16_t lo, int16_t hi)
{
	return (uint16_t)lo | ((uint32_t)(uint16_t)hi << 16);
}

static inline int32_t sat_5bit(int16_t v)
{
	int32_t c = v >> 7;
	return c < 0 ? 0 : c > 0x1F ? 0x1F : c;
}

uint32_t PC_GteGetData(int reg)
{
	switch (reg & 31) {
	case 0: return pack16(g.v[0][0], g.v[0][1]);
	case 1: return (uint32_t)(int32_t)g.v[0][2];
	case 2: return pack16(g.v[1][0], g.v[1][1]);
	case 3: return (uint32_t)(int32_t)g.v[1][2];
	case 4: return pack16(g.v[2][0], g.v[2][1]);
	case 5: return (uint32_t)(int32_t)g.v[2][2];
	case 6:
		return g.rgbc[0] | (g.rgbc[1] << 8) | (g.rgbc[2] << 16) | ((uint32_t)g.rgbc[3] << 24);
	case 7: return g.otz;
	case 8: case 9: case 10: case 11: return (uint32_t)(int32_t)g.ir[reg - 8];
	case 12: case 13: case 14: return pack16(g.sx[reg - 12], g.sy[reg - 12]);
	case 15: return pack16(g.sx[2], g.sy[2]);
	case 16: case 17: case 18: case 19: return g.sz[reg - 16];
	case 20: case 21: case 22: return g.rgb[reg - 20];
	case 23: return g.res1;
	case 24: case 25: case 26: case 27: return (uint32_t)g.mac[reg - 24];
	case 28:
	case 29:
		return (uint32_t)(sat_5bit(g.ir[1]) | (sat_5bit(g.ir[2]) << 5) | (sat_5bit(g.ir[3]) << 10));
	case 30: return g.lzcs;
	default: return g.lzcr;
	}
}

static uint32_t count_lead(uint32_t v)
{
	if (v & 0x80000000u)
		v = ~v;
	uint32_t n = 0;
	while (n < 32 && !(v & (0x80000000u >> n)))
		n++;
	return n;
}

void PC_GteSetData(int reg, uint32_t value)
{
	switch (reg & 31) {
	case 0: case 2: case 4:
		g.v[reg / 2][0] = (int16_t)value;
		g.v[reg / 2][1] = (int16_t)(value >> 16);
		break;
	case 1: case 3: case 5: g.v[reg / 2][2] = (int16_t)value; break;
	case 6:
		g.rgbc[0] = (uint8_t)value;
		g.rgbc[1] = (uint8_t)(value >> 8);
		g.rgbc[2] = (uint8_t)(value >> 16);
		g.rgbc[3] = (uint8_t)(value >> 24);
		break;
	case 7: g.otz = (uint16_t)value; break;
	case 8: case 9: case 10: case 11: g.ir[reg - 8] = (int16_t)value; break;
	case 12: case 13: case 14:
		g.sx[reg - 12] = (int16_t)value;
		g.sy[reg - 12] = (int16_t)(value >> 16);
		break;
	case 15:
		g.sx[0] = g.sx[1];
		g.sy[0] = g.sy[1];
		g.sx[1] = g.sx[2];
		g.sy[1] = g.sy[2];
		g.sx[2] = (int16_t)value;
		g.sy[2] = (int16_t)(value >> 16);
		break;
	case 16: case 17: case 18: case 19: g.sz[reg - 16] = (uint16_t)value; break;
	case 20: case 21: case 22: g.rgb[reg - 20] = value; break;
	case 23: g.res1 = value; break;
	case 24: case 25: case 26: case 27: g.mac[reg - 24] = (int32_t)value; break;
	case 28:
		g.ir[1] = (int16_t)((value & 0x1F) << 7);
		g.ir[2] = (int16_t)(((value >> 5) & 0x1F) << 7);
		g.ir[3] = (int16_t)(((value >> 10) & 0x1F) << 7);
		break;
	case 29: break;
	case 30:
		g.lzcs = value;
		g.lzcr = count_lead(value);
		break;
	default: break;
	}
}

static int16_t *ctrl_matrix(int reg)
{
	switch (reg >> 3) {
	case 0: return &g.rt[0][0];
	case 1: return &g.llm[0][0];
	default: return &g.lcm[0][0];
	}
}

static int32_t *ctrl_vector(int reg)
{
	switch (reg >> 3) {
	case 0: return g.tr;
	case 1: return g.bk;
	default: return g.fc;
	}
}

uint32_t PC_GteGetCtrl(int reg)
{
	reg &= 31;
	if (reg < 24) {
		int k = reg & 7;
		if (k < 4) {
			int16_t *m = ctrl_matrix(reg);
			return pack16(m[2 * k], m[2 * k + 1]);
		}
		if (k == 4)
			return (uint32_t)(int32_t)ctrl_matrix(reg)[8];
		return (uint32_t)ctrl_vector(reg)[k - 5];
	}
	switch (reg) {
	case 24: return (uint32_t)g.ofx;
	case 25: return (uint32_t)g.ofy;
	case 26: return (uint32_t)(int32_t)(int16_t)g.h; /* hardware sign-extends H on read */
	case 27: return (uint32_t)(int32_t)g.dqa;
	case 28: return (uint32_t)g.dqb;
	case 29: return (uint32_t)(int32_t)g.zsf3;
	case 30: return (uint32_t)(int32_t)g.zsf4;
	default: return g.flag;
	}
}

void PC_GteSetCtrl(int reg, uint32_t value)
{
	reg &= 31;
	if (reg < 24) {
		int k = reg & 7;
		if (k < 4) {
			int16_t *m = ctrl_matrix(reg);
			m[2 * k] = (int16_t)value;
			m[2 * k + 1] = (int16_t)(value >> 16);
		} else if (k == 4) {
			ctrl_matrix(reg)[8] = (int16_t)value;
		} else {
			ctrl_vector(reg)[k - 5] = (int32_t)value;
		}
		return;
	}
	switch (reg) {
	case 24: g.ofx = (int32_t)value; break;
	case 25: g.ofy = (int32_t)value; break;
	case 26: g.h = (uint16_t)value; break;
	case 27: g.dqa = (int16_t)value; break;
	case 28: g.dqb = (int32_t)value; break;
	case 29: g.zsf3 = (int16_t)value; break;
	case 30: g.zsf4 = (int16_t)value; break;
	default:
		g.flag = value & 0x7FFFF000u;
		if (g.flag & F_ERROR_MASK)
			g.flag |= 0x80000000u;
		break;
	}
}

void PC_GteReset(void)
{
	memset(&g, 0, sizeof g);
}

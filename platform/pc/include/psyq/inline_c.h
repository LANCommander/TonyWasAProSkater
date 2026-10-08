/* GTE macros with the Psy-Q INLINE_C.H names, implemented on the software GTE (pc_gte.h).
 * Each macro performs the same register transfers as its PS1 counterpart: lwc2/swc2 become a
 * 32-bit memory access plus PC_GteSetData/PC_GteGetData, mtc2/mfc2/ctc2/cfc2 become register
 * accesses, and each GTE command runs PC_GteCommand with its real cop2 command word.
 * Pointer arguments are evaluated once; value arguments are converted to 32 bits. */
#ifndef PSYQ_INLINE_C_H
#define PSYQ_INLINE_C_H

#include <stdint.h>

#include "../pc_gte.h"

/* ---- memory helpers -------------------------------------------------------------------------- */

static __inline__ uint32_t pcgte_lw(const void *p, int off)
{
	uint32_t v;
	__builtin_memcpy(&v, (const unsigned char *)p + off, 4);
	return v;
}
static __inline__ uint32_t pcgte_lhu(const void *p, int off)
{
	uint16_t v;
	__builtin_memcpy(&v, (const unsigned char *)p + off, 2);
	return v;
}
static __inline__ uint32_t pcgte_lh(const void *p, int off)
{
	int16_t v;
	__builtin_memcpy(&v, (const unsigned char *)p + off, 2);
	return (uint32_t)(int32_t)v;
}
static __inline__ uint32_t pcgte_lbu(const void *p, int off)
{
	return ((const unsigned char *)p)[off];
}
static __inline__ void pcgte_sw(void *p, int off, uint32_t v)
{
	__builtin_memcpy((unsigned char *)p + off, &v, 4);
}
static __inline__ void pcgte_sh(void *p, int off, uint32_t v)
{
	uint16_t h = (uint16_t)v;
	__builtin_memcpy((unsigned char *)p + off, &h, 2);
}
static __inline__ void pcgte_sb(void *p, int off, uint32_t v)
{
	((unsigned char *)p)[off] = (unsigned char)v;
}
static __inline__ void pcgte_lwc2(int reg, const void *p, int off)
{
	PC_GteSetData(reg, pcgte_lw(p, off));
}
static __inline__ void pcgte_swc2(int reg, void *p, int off)
{
	pcgte_sw(p, off, PC_GteGetData(reg));
}
static __inline__ void pcgte_ctc2_lw(int reg, const void *p, int off)
{
	PC_GteSetCtrl(reg, pcgte_lw(p, off));
}
static __inline__ void pcgte_cfc2_sw(int reg, void *p, int off)
{
	pcgte_sw(p, off, PC_GteGetCtrl(reg));
}

#define PCGTE_P(x) ((const void *)(x))
#define PCGTE_W(x) ((void *)(x))
#define PCGTE_V(x) ((uint32_t)(x))

/* Statement wrappers: one pointer / one value bound once. */
#define PCGTE_LD1(r0, body)                                                                        \
	do {                                                                                           \
		const void *pcgte_a0_ = PCGTE_P(r0);                                                       \
		body;                                                                                      \
	} while (0)
#define PCGTE_ST1(r0, body)                                                                        \
	do {                                                                                           \
		void *pcgte_a0_ = PCGTE_W(r0);                                                             \
		body;                                                                                      \
	} while (0)

/* ---- loads (type 1) -------------------------------------------------------------------------- */

#define gte_ldv0(r0) PCGTE_LD1(r0, pcgte_lwc2(0, pcgte_a0_, 0); pcgte_lwc2(1, pcgte_a0_, 4))
#define gte_ldv1(r0) PCGTE_LD1(r0, pcgte_lwc2(2, pcgte_a0_, 0); pcgte_lwc2(3, pcgte_a0_, 4))
#define gte_ldv2(r0) PCGTE_LD1(r0, pcgte_lwc2(4, pcgte_a0_, 0); pcgte_lwc2(5, pcgte_a0_, 4))

#define gte_ldv3(r0, r1, r2)                                                                       \
	do {                                                                                           \
		const void *pcgte_a0_ = PCGTE_P(r0);                                                       \
		const void *pcgte_a1_ = PCGTE_P(r1);                                                       \
		const void *pcgte_a2_ = PCGTE_P(r2);                                                       \
		pcgte_lwc2(0, pcgte_a0_, 0);                                                               \
		pcgte_lwc2(1, pcgte_a0_, 4);                                                               \
		pcgte_lwc2(2, pcgte_a1_, 0);                                                               \
		pcgte_lwc2(3, pcgte_a1_, 4);                                                               \
		pcgte_lwc2(4, pcgte_a2_, 0);                                                               \
		pcgte_lwc2(5, pcgte_a2_, 4);                                                               \
	} while (0)

#define gte_ldv3c(r0)                                                                              \
	PCGTE_LD1(r0, pcgte_lwc2(0, pcgte_a0_, 0); pcgte_lwc2(1, pcgte_a0_, 4);                        \
	          pcgte_lwc2(2, pcgte_a0_, 8); pcgte_lwc2(3, pcgte_a0_, 12);                           \
	          pcgte_lwc2(4, pcgte_a0_, 16); pcgte_lwc2(5, pcgte_a0_, 20))

#define gte_ldv3c_vertc(r0)                                                                        \
	PCGTE_LD1(r0, pcgte_lwc2(0, pcgte_a0_, 0); pcgte_lwc2(1, pcgte_a0_, 4);                        \
	          pcgte_lwc2(2, pcgte_a0_, 12); pcgte_lwc2(3, pcgte_a0_, 16);                          \
	          pcgte_lwc2(4, pcgte_a0_, 24); pcgte_lwc2(5, pcgte_a0_, 28))

#define gte_ldv01(r0, r1)                                                                          \
	do {                                                                                           \
		const void *pcgte_a0_ = PCGTE_P(r0);                                                       \
		const void *pcgte_a1_ = PCGTE_P(r1);                                                       \
		pcgte_lwc2(0, pcgte_a0_, 0);                                                               \
		pcgte_lwc2(1, pcgte_a0_, 4);                                                               \
		pcgte_lwc2(2, pcgte_a1_, 0);                                                               \
		pcgte_lwc2(3, pcgte_a1_, 4);                                                               \
	} while (0)

#define gte_ldv01c(r0)                                                                             \
	PCGTE_LD1(r0, pcgte_lwc2(0, pcgte_a0_, 0); pcgte_lwc2(1, pcgte_a0_, 4);                        \
	          pcgte_lwc2(2, pcgte_a0_, 8); pcgte_lwc2(3, pcgte_a0_, 12))

#define gte_ldrgb(r0) PCGTE_LD1(r0, pcgte_lwc2(6, pcgte_a0_, 0))

#define gte_ldrgb3(r0, r1, r2)                                                                     \
	do {                                                                                           \
		const void *pcgte_a0_ = PCGTE_P(r0);                                                       \
		const void *pcgte_a1_ = PCGTE_P(r1);                                                       \
		const void *pcgte_a2_ = PCGTE_P(r2);                                                       \
		pcgte_lwc2(20, pcgte_a0_, 0);                                                              \
		pcgte_lwc2(21, pcgte_a1_, 0);                                                              \
		pcgte_lwc2(22, pcgte_a2_, 0);                                                              \
		pcgte_lwc2(6, pcgte_a2_, 0);                                                               \
	} while (0)

#define gte_ldrgb3c(r0)                                                                            \
	PCGTE_LD1(r0, pcgte_lwc2(20, pcgte_a0_, 0); pcgte_lwc2(21, pcgte_a0_, 4);                      \
	          pcgte_lwc2(22, pcgte_a0_, 8); pcgte_lwc2(6, pcgte_a0_, 8))

/* VECTOR (32-bit components) -> V0 */
#define gte_ldlv0(r0)                                                                              \
	PCGTE_LD1(r0, PC_GteSetData(0, pcgte_lhu(pcgte_a0_, 0) | (pcgte_lhu(pcgte_a0_, 4) << 16));     \
	          pcgte_lwc2(1, pcgte_a0_, 8))

/* VECTOR -> IR1..IR3 */
#define gte_ldlvl(r0)                                                                              \
	PCGTE_LD1(r0, pcgte_lwc2(9, pcgte_a0_, 0); pcgte_lwc2(10, pcgte_a0_, 4);                       \
	          pcgte_lwc2(11, pcgte_a0_, 8))

/* SVECTOR -> IR1..IR3 */
#define gte_ldsv(r0)                                                                               \
	PCGTE_LD1(r0, PC_GteSetData(9, pcgte_lhu(pcgte_a0_, 0));                                       \
	          PC_GteSetData(10, pcgte_lhu(pcgte_a0_, 2));                                          \
	          PC_GteSetData(11, pcgte_lhu(pcgte_a0_, 4)))

#define gte_ldbv(r0)                                                                               \
	PCGTE_LD1(r0, PC_GteSetData(9, pcgte_lbu(pcgte_a0_, 0));                                       \
	          PC_GteSetData(10, pcgte_lbu(pcgte_a0_, 1)))

#define gte_ldcv(r0)                                                                               \
	PCGTE_LD1(r0, PC_GteSetData(9, pcgte_lbu(pcgte_a0_, 0));                                       \
	          PC_GteSetData(10, pcgte_lbu(pcgte_a0_, 1));                                          \
	          PC_GteSetData(11, pcgte_lbu(pcgte_a0_, 2)))

/* MATRIX column 0 -> IR1..IR3 */
#define gte_ldclmv(r0)                                                                             \
	PCGTE_LD1(r0, PC_GteSetData(9, pcgte_lhu(pcgte_a0_, 0));                                       \
	          PC_GteSetData(10, pcgte_lhu(pcgte_a0_, 6));                                          \
	          PC_GteSetData(11, pcgte_lhu(pcgte_a0_, 12)))

#define gte_lddp(r0) PC_GteSetData(8, PCGTE_V(r0))
#define gte_ldsxy0(r0) PC_GteSetData(12, PCGTE_V(r0))
#define gte_ldsxy1(r0) PC_GteSetData(13, PCGTE_V(r0))
#define gte_ldsxy2(r0) PC_GteSetData(14, PCGTE_V(r0))

#define gte_ldsxy3(r0, r1, r2)                                                                     \
	do {                                                                                           \
		uint32_t pcgte_v0_ = PCGTE_V(r0), pcgte_v1_ = PCGTE_V(r1), pcgte_v2_ = PCGTE_V(r2);        \
		PC_GteSetData(12, pcgte_v0_);                                                              \
		PC_GteSetData(14, pcgte_v2_);                                                              \
		PC_GteSetData(13, pcgte_v1_);                                                              \
	} while (0)

#define gte_ldsxy3c(r0)                                                                            \
	PCGTE_LD1(r0, pcgte_lwc2(12, pcgte_a0_, 0); pcgte_lwc2(13, pcgte_a0_, 4);                      \
	          pcgte_lwc2(14, pcgte_a0_, 8))

#define gte_ldsz3(r0, r1, r2)                                                                      \
	do {                                                                                           \
		uint32_t pcgte_v0_ = PCGTE_V(r0), pcgte_v1_ = PCGTE_V(r1), pcgte_v2_ = PCGTE_V(r2);        \
		PC_GteSetData(17, pcgte_v0_);                                                              \
		PC_GteSetData(18, pcgte_v1_);                                                              \
		PC_GteSetData(19, pcgte_v2_);                                                              \
	} while (0)

#define gte_ldsz4(r0, r1, r2, r3)                                                                  \
	do {                                                                                           \
		uint32_t pcgte_v0_ = PCGTE_V(r0), pcgte_v1_ = PCGTE_V(r1), pcgte_v2_ = PCGTE_V(r2),        \
		         pcgte_v3_ = PCGTE_V(r3);                                                          \
		PC_GteSetData(16, pcgte_v0_);                                                              \
		PC_GteSetData(17, pcgte_v1_);                                                              \
		PC_GteSetData(18, pcgte_v2_);                                                              \
		PC_GteSetData(19, pcgte_v3_);                                                              \
	} while (0)

/* Outer product: VECTOR v1 -> RT11/RT22/RT33 (as whole words), VECTOR v2 -> IR1..IR3 */
#define gte_ldopv1(r0)                                                                             \
	PCGTE_LD1(r0, pcgte_ctc2_lw(0, pcgte_a0_, 0); pcgte_ctc2_lw(2, pcgte_a0_, 4);                  \
	          pcgte_ctc2_lw(4, pcgte_a0_, 8))
#define gte_ldopv2(r0)                                                                             \
	PCGTE_LD1(r0, pcgte_lwc2(11, pcgte_a0_, 8); pcgte_lwc2(9, pcgte_a0_, 0);                       \
	          pcgte_lwc2(10, pcgte_a0_, 4))
#define gte_ldopv1SV(r0)                                                                           \
	PCGTE_LD1(r0, PC_GteSetCtrl(0, pcgte_lh(pcgte_a0_, 0)); PC_GteSetCtrl(2, pcgte_lh(pcgte_a0_, 2)); \
	          PC_GteSetCtrl(4, pcgte_lh(pcgte_a0_, 4)))
#define gte_ldopv2SV(r0)                                                                           \
	PCGTE_LD1(r0, PC_GteSetData(9, pcgte_lh(pcgte_a0_, 0)); PC_GteSetData(10, pcgte_lh(pcgte_a0_, 2)); \
	          PC_GteSetData(11, pcgte_lh(pcgte_a0_, 4)))

#define gte_ldlzc(r0) PC_GteSetData(30, PCGTE_V(r0))

#define gte_SetRGBcd(r0) PCGTE_LD1(r0, pcgte_lwc2(6, pcgte_a0_, 0))

#define PCGTE_CTC3(base, r0, r1, r2, shl)                                                          \
	do {                                                                                           \
		uint32_t pcgte_v0_ = PCGTE_V(r0), pcgte_v1_ = PCGTE_V(r1), pcgte_v2_ = PCGTE_V(r2);        \
		PC_GteSetCtrl((base), pcgte_v0_ << (shl));                                                 \
		PC_GteSetCtrl((base) + 1, pcgte_v1_ << (shl));                                             \
		PC_GteSetCtrl((base) + 2, pcgte_v2_ << (shl));                                             \
	} while (0)

#define gte_ldbkdir(r0, r1, r2) PCGTE_CTC3(13, r0, r1, r2, 0)
#define gte_SetBackColor(r0, r1, r2) PCGTE_CTC3(13, r0, r1, r2, 4)
#define gte_ldfcdir(r0, r1, r2) PCGTE_CTC3(21, r0, r1, r2, 0)
#define gte_SetFarColor(r0, r1, r2) PCGTE_CTC3(21, r0, r1, r2, 4)
#define gte_ldtr(r0, r1, r2) PCGTE_CTC3(5, r0, r1, r2, 0)

#define gte_SetGeomOffset(r0, r1)                                                                  \
	do {                                                                                           \
		uint32_t pcgte_v0_ = PCGTE_V(r0), pcgte_v1_ = PCGTE_V(r1);                                 \
		PC_GteSetCtrl(24, pcgte_v0_ << 16);                                                        \
		PC_GteSetCtrl(25, pcgte_v1_ << 16);                                                        \
	} while (0)

#define gte_SetGeomScreen(r0) PC_GteSetCtrl(26, PCGTE_V(r0))

/* MATRIX m[3][3] (5 words from offset 0) -> control registers base..base+4 */
#define PCGTE_LDMAT(base, r0)                                                                      \
	PCGTE_LD1(r0, pcgte_ctc2_lw((base), pcgte_a0_, 0); pcgte_ctc2_lw((base) + 1, pcgte_a0_, 4);    \
	          pcgte_ctc2_lw((base) + 2, pcgte_a0_, 8); pcgte_ctc2_lw((base) + 3, pcgte_a0_, 12);   \
	          pcgte_ctc2_lw((base) + 4, pcgte_a0_, 16))
#define PCGTE_LDROW0(base, r0)                                                                     \
	PCGTE_LD1(r0, pcgte_ctc2_lw((base), pcgte_a0_, 0); pcgte_ctc2_lw((base) + 1, pcgte_a0_, 4))

#define gte_ldsvrtrow0(r0) PCGTE_LDROW0(0, r0)
#define gte_SetRotMatrix(r0) PCGTE_LDMAT(0, r0)
#define gte_ldsvllrow0(r0) PCGTE_LDROW0(8, r0)
#define gte_SetLightMatrix(r0) PCGTE_LDMAT(8, r0)
#define gte_ldsvlcrow0(r0) PCGTE_LDROW0(16, r0)
#define gte_SetColorMatrix(r0) PCGTE_LDMAT(16, r0)

#define gte_SetTransMatrix(r0)                                                                     \
	PCGTE_LD1(r0, pcgte_ctc2_lw(5, pcgte_a0_, 20); pcgte_ctc2_lw(6, pcgte_a0_, 24);                \
	          pcgte_ctc2_lw(7, pcgte_a0_, 28))
#define gte_SetTransVector(r0)                                                                     \
	PCGTE_LD1(r0, pcgte_ctc2_lw(5, pcgte_a0_, 0); pcgte_ctc2_lw(6, pcgte_a0_, 4);                  \
	          pcgte_ctc2_lw(7, pcgte_a0_, 8))
#define gte_ldfc(r0)                                                                               \
	PCGTE_LD1(r0, pcgte_ctc2_lw(21, pcgte_a0_, 0); pcgte_ctc2_lw(22, pcgte_a0_, 4);                \
	          pcgte_ctc2_lw(23, pcgte_a0_, 8))

#define gte_ld_intpol_uv0(r0)                                                                      \
	PCGTE_LD1(r0, PC_GteSetCtrl(21, pcgte_lbu(pcgte_a0_, 0)); PC_GteSetCtrl(22, pcgte_lbu(pcgte_a0_, 1)))
#define gte_ld_intpol_uv1(r0)                                                                      \
	PCGTE_LD1(r0, PC_GteSetData(9, pcgte_lbu(pcgte_a0_, 0)); PC_GteSetData(10, pcgte_lbu(pcgte_a0_, 1)))
#define gte_ld_intpol_bv0(r0) gte_ld_intpol_uv0(r0)
#define gte_ld_intpol_bv1(r0) gte_ld_intpol_uv1(r0)
#define gte_ld_intpol_sv0(r0)                                                                      \
	PCGTE_LD1(r0, PC_GteSetCtrl(21, pcgte_lh(pcgte_a0_, 0)); PC_GteSetCtrl(22, pcgte_lh(pcgte_a0_, 2)); \
	          PC_GteSetCtrl(23, pcgte_lh(pcgte_a0_, 4)))
#define gte_ld_intpol_sv1(r0)                                                                      \
	PCGTE_LD1(r0, PC_GteSetData(9, pcgte_lh(pcgte_a0_, 0)); PC_GteSetData(10, pcgte_lh(pcgte_a0_, 2)); \
	          PC_GteSetData(11, pcgte_lh(pcgte_a0_, 4)))

/* Not in Psy-Q: single register moves the decompiled sources use. */
#ifndef gte_ldVXY0
#define gte_ldVXY0(r0) PC_GteSetData(0, PCGTE_V(r0))
#define gte_ldVZ0(r0) PC_GteSetData(1, PCGTE_V(r0))
#define gte_ldVXY1(r0) PC_GteSetData(2, PCGTE_V(r0))
#define gte_ldVZ1(r0) PC_GteSetData(3, PCGTE_V(r0))
#define gte_ldVXY2(r0) PC_GteSetData(4, PCGTE_V(r0))
#define gte_ldVZ2(r0) PC_GteSetData(5, PCGTE_V(r0))
#endif
#ifndef gte_ldRGBC_r
#define gte_ldRGBC_r(r0) PC_GteSetData(6, PCGTE_V(r0))
#endif
#ifndef gte_ldlvnl
#define gte_ldlvnl(r0)                                                                             \
	PCGTE_LD1(r0, pcgte_lwc2(25, pcgte_a0_, 0); pcgte_lwc2(26, pcgte_a0_, 4);                      \
	          pcgte_lwc2(27, pcgte_a0_, 8))
#endif

/* ---- commands (type 2) ----------------------------------------------------------------------- */

/* MVMVA: sf, mx (0 RT, 1 LLM, 2 LCM), v (0-2 V0-V2, 3 IR), cv (0 TR, 1 BK, 2 FC, 3 none), lm */
#define PCGTE_MVMVA(sf, mx, v, cv, lm)                                                             \
	(0x0400012u | ((uint32_t)(sf) << 19) | ((uint32_t)(mx) << 17) | ((uint32_t)(v) << 15) |        \
	 ((uint32_t)(cv) << 13) | ((uint32_t)(lm) << 10))

#define gte_rtps() PC_GteCommand(0x0180001)
#define gte_rtpt() PC_GteCommand(0x0280030)
#define gte_rt() PC_GteCommand(0x0480012)
#define gte_rtv0() PC_GteCommand(0x0486012)
#define gte_rtv1() PC_GteCommand(0x048e012)
#define gte_rtv2() PC_GteCommand(0x0496012)
#define gte_rtir() PC_GteCommand(0x049e012)
#define gte_rtir_sf0() PC_GteCommand(0x041e012)
#define gte_rtv0tr() PC_GteCommand(0x0480012)
#define gte_rtv1tr() PC_GteCommand(0x0488012)
#define gte_rtv2tr() PC_GteCommand(0x0490012)
#define gte_rtirtr() PC_GteCommand(0x0498012)
#define gte_rtv0bk() PC_GteCommand(0x0482012)
#define gte_rtv1bk() PC_GteCommand(0x048a012)
#define gte_rtv2bk() PC_GteCommand(0x0492012)
#define gte_rtirbk() PC_GteCommand(0x049a012)
#define gte_ll() PC_GteCommand(0x04a6412)
#define gte_llv0() PC_GteCommand(0x04a6012)
#define gte_llv1() PC_GteCommand(0x04ae012)
#define gte_llv2() PC_GteCommand(0x04b6012)
#define gte_llir() PC_GteCommand(0x04be012)
#define gte_llv0tr() PC_GteCommand(0x04a0012)
#define gte_llv1tr() PC_GteCommand(0x04a8012)
#define gte_llv2tr() PC_GteCommand(0x04b0012)
#define gte_llirtr() PC_GteCommand(0x04b8012)
#define gte_llv0bk() PC_GteCommand(0x04a2012)
#define gte_llv1bk() PC_GteCommand(0x04aa012)
#define gte_llv2bk() PC_GteCommand(0x04b2012)
#define gte_llirbk() PC_GteCommand(0x04ba012)
#define gte_lc() PC_GteCommand(0x04da412)
#define gte_lcv0() PC_GteCommand(0x04c6012)
#define gte_lcv1() PC_GteCommand(0x04ce012)
#define gte_lcv2() PC_GteCommand(0x04d6012)
#define gte_lcir() PC_GteCommand(0x04de012)
#define gte_lcv0tr() PC_GteCommand(0x04c0012)
#define gte_lcv1tr() PC_GteCommand(0x04c8012)
#define gte_lcv2tr() PC_GteCommand(0x04d0012)
#define gte_lcirtr() PC_GteCommand(0x04d8012)
#define gte_lcv0bk() PC_GteCommand(0x04c2012)
#define gte_lcv1bk() PC_GteCommand(0x04ca012)
#define gte_lcv2bk() PC_GteCommand(0x04d2012)
#define gte_lcirbk() PC_GteCommand(0x04da012)
#define gte_dpcl() PC_GteCommand(0x0680029)
#define gte_dpcs() PC_GteCommand(0x0780010)
#define gte_dpct() PC_GteCommand(0x0f8002a)
#define gte_intpl() PC_GteCommand(0x0980011)
#define gte_sqr12() PC_GteCommand(0x0a80428)
#define gte_sqr0() PC_GteCommand(0x0a00428)
#define gte_ncs() PC_GteCommand(0x0c8041e)
#define gte_nct() PC_GteCommand(0x0d80420)
#define gte_ncds() PC_GteCommand(0x0e80413)
#define gte_ncdt() PC_GteCommand(0x0f80416)
#define gte_nccs() PC_GteCommand(0x108041b)
#define gte_ncct() PC_GteCommand(0x118043f)
#define gte_cdp() PC_GteCommand(0x1280414)
#define gte_cc() PC_GteCommand(0x138041c)
#define gte_nclip() PC_GteCommand(0x1400006)
#define gte_avsz3() PC_GteCommand(0x158002d)
#define gte_avsz4() PC_GteCommand(0x168002e)
#define gte_op12() PC_GteCommand(0x178000c)
#define gte_op0() PC_GteCommand(0x170000c)
#define gte_gpf12() PC_GteCommand(0x198003d)
#define gte_gpf0() PC_GteCommand(0x190003d)
#define gte_gpl12() PC_GteCommand(0x1a8003e)
#define gte_gpl0() PC_GteCommand(0x1a0003e)
#define gte_mvmva(sf, mx, v, cv, lm) PC_GteCommand(PCGTE_MVMVA(sf, mx, v, cv, lm))

/* The core form takes the DMPSX placeholder word (0x13bf | sf<<25 | mx<<23 | v<<21 | cv<<19 |
 * lm<<18) that gte_mvmva builds on the PS1. */
#define gte_mvmva_core(w)                                                                          \
	do {                                                                                           \
		uint32_t pcgte_w_ = PCGTE_V(w);                                                            \
		PC_GteCommand(PCGTE_MVMVA((pcgte_w_ >> 25) & 1, (pcgte_w_ >> 23) & 3,                      \
		                          (pcgte_w_ >> 21) & 3, (pcgte_w_ >> 19) & 3,                      \
		                          (pcgte_w_ >> 18) & 1));                                          \
	} while (0)

/* Variants without the leading nops: identical here. */
#define gte_rtps_b() gte_rtps()
#define gte_rtpt_b() gte_rtpt()
#define gte_rt_b() gte_rt()
#define gte_rtv0_b() gte_rtv0()
#define gte_rtv1_b() gte_rtv1()
#define gte_rtv2_b() gte_rtv2()
#define gte_rtir_b() gte_rtir()
#define gte_rtir_sf0_b() gte_rtir_sf0()
#define gte_rtv0tr_b() gte_rtv0tr()
#define gte_rtv1tr_b() gte_rtv1tr()
#define gte_rtv2tr_b() gte_rtv2tr()
#define gte_rtirtr_b() gte_rtirtr()
#define gte_rtv0bk_b() gte_rtv0bk()
#define gte_rtv1bk_b() gte_rtv1bk()
#define gte_rtv2bk_b() gte_rtv2bk()
#define gte_rtirbk_b() gte_rtirbk()
#define gte_ll_b() gte_ll()
#define gte_llv0_b() gte_llv0()
#define gte_llv1_b() gte_llv1()
#define gte_llv2_b() gte_llv2()
#define gte_llir_b() gte_llir()
#define gte_llv0tr_b() gte_llv0tr()
#define gte_llv1tr_b() gte_llv1tr()
#define gte_llv2tr_b() gte_llv2tr()
#define gte_llirtr_b() gte_llirtr()
#define gte_llv0bk_b() gte_llv0bk()
#define gte_llv1bk_b() gte_llv1bk()
#define gte_llv2bk_b() gte_llv2bk()
#define gte_llirbk_b() gte_llirbk()
#define gte_lc_b() gte_lc()
#define gte_lcv0_b() gte_lcv0()
#define gte_lcv1_b() gte_lcv1()
#define gte_lcv2_b() gte_lcv2()
#define gte_lcir_b() gte_lcir()
#define gte_lcv0tr_b() gte_lcv0tr()
#define gte_lcv1tr_b() gte_lcv1tr()
#define gte_lcv2tr_b() gte_lcv2tr()
#define gte_lcirtr_b() gte_lcirtr()
#define gte_lcv0bk_b() gte_lcv0bk()
#define gte_lcv1bk_b() gte_lcv1bk()
#define gte_lcv2bk_b() gte_lcv2bk()
#define gte_lcirbk_b() gte_lcirbk()
#define gte_dpcl_b() gte_dpcl()
#define gte_dpcs_b() gte_dpcs()
#define gte_dpct_b() gte_dpct()
#define gte_intpl_b() gte_intpl()
#define gte_sqr12_b() gte_sqr12()
#define gte_sqr0_b() gte_sqr0()
#define gte_ncs_b() gte_ncs()
#define gte_nct_b() gte_nct()
#define gte_ncds_b() gte_ncds()
#define gte_ncdt_b() gte_ncdt()
#define gte_nccs_b() gte_nccs()
#define gte_ncct_b() gte_ncct()
#define gte_cdp_b() gte_cdp()
#define gte_cc_b() gte_cc()
#define gte_nclip_b() gte_nclip()
#define gte_avsz3_b() gte_avsz3()
#define gte_avsz4_b() gte_avsz4()
#define gte_op12_b() gte_op12()
#define gte_op0_b() gte_op0()
#define gte_gpf12_b() gte_gpf12()
#define gte_gpf0_b() gte_gpf0()
#define gte_gpl12_b() gte_gpl12()
#define gte_gpl0_b() gte_gpl0()
#define gte_mvmva_core_b(w) gte_mvmva_core(w)
#define gte_mvmva_b(sf, mx, v, cv, lm) gte_mvmva(sf, mx, v, cv, lm)

/* ---- stores (type 3) ------------------------------------------------------------------------- */

#define PCGTE_SWC2_3(ra, rb, rc, r0, r1, r2)                                                       \
	do {                                                                                           \
		void *pcgte_a0_ = PCGTE_W(r0);                                                             \
		void *pcgte_a1_ = PCGTE_W(r1);                                                             \
		void *pcgte_a2_ = PCGTE_W(r2);                                                             \
		pcgte_swc2((ra), pcgte_a0_, 0);                                                            \
		pcgte_swc2((rb), pcgte_a1_, 0);                                                            \
		pcgte_swc2((rc), pcgte_a2_, 0);                                                            \
	} while (0)
#define PCGTE_SWC2_3OFF(ra, rb, rc, r0, oa, ob, oc)                                                \
	PCGTE_ST1(r0, pcgte_swc2((ra), pcgte_a0_, (oa)); pcgte_swc2((rb), pcgte_a0_, (ob));            \
	          pcgte_swc2((rc), pcgte_a0_, (oc)))

#define gte_stsxy(r0) PCGTE_ST1(r0, pcgte_swc2(14, pcgte_a0_, 0))
#define gte_stsxy3(r0, r1, r2) PCGTE_SWC2_3(12, 13, 14, r0, r1, r2)
#define gte_stsxy3c(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 0, 4, 8)
#define gte_stsxy2(r0) PCGTE_ST1(r0, pcgte_swc2(14, pcgte_a0_, 0))
#define gte_stsxy1(r0) PCGTE_ST1(r0, pcgte_swc2(13, pcgte_a0_, 0))
#define gte_stsxy0(r0) PCGTE_ST1(r0, pcgte_swc2(12, pcgte_a0_, 0))
#define gte_stsxy01(r0, r1)                                                                        \
	do {                                                                                           \
		void *pcgte_a0_ = PCGTE_W(r0);                                                             \
		void *pcgte_a1_ = PCGTE_W(r1);                                                             \
		pcgte_swc2(12, pcgte_a0_, 0);                                                              \
		pcgte_swc2(13, pcgte_a1_, 0);                                                              \
	} while (0)
#define gte_stsxy01c(r0) PCGTE_ST1(r0, pcgte_swc2(12, pcgte_a0_, 0); pcgte_swc2(13, pcgte_a0_, 4))

/* Screen XY straight into GPU primitives (offsets of the x0/x1/x2 fields). */
#define gte_stsxy3_f3(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 8, 12, 16)
#define gte_stsxy3_g3(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 8, 16, 24)
#define gte_stsxy3_ft3(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 8, 16, 24)
#define gte_stsxy3_gt3(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 8, 20, 32)
#define gte_stsxy3_f4(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 8, 12, 16)
#define gte_stsxy3_g4(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 8, 16, 24)
#define gte_stsxy3_ft4(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 8, 16, 24)
#define gte_stsxy3_gt4(r0) PCGTE_SWC2_3OFF(12, 13, 14, r0, 8, 20, 32)

#define gte_stdp(r0) PCGTE_ST1(r0, pcgte_swc2(8, pcgte_a0_, 0))
#define gte_stflg(r0) PCGTE_ST1(r0, pcgte_cfc2_sw(31, pcgte_a0_, 0))
#define gte_stflg_4(r0) PCGTE_ST1(r0, pcgte_sw(pcgte_a0_, 0, PC_GteGetCtrl(31) & 0x40000u))
#define gte_stsz(r0) PCGTE_ST1(r0, pcgte_swc2(19, pcgte_a0_, 0))
#define gte_stsz3(r0, r1, r2) PCGTE_SWC2_3(17, 18, 19, r0, r1, r2)
#define gte_stsz4(r0, r1, r2, r3)                                                                  \
	do {                                                                                           \
		void *pcgte_a0_ = PCGTE_W(r0);                                                             \
		void *pcgte_a1_ = PCGTE_W(r1);                                                             \
		void *pcgte_a2_ = PCGTE_W(r2);                                                             \
		void *pcgte_a3_ = PCGTE_W(r3);                                                             \
		pcgte_swc2(16, pcgte_a0_, 0);                                                              \
		pcgte_swc2(17, pcgte_a1_, 0);                                                              \
		pcgte_swc2(18, pcgte_a2_, 0);                                                              \
		pcgte_swc2(19, pcgte_a3_, 0);                                                              \
	} while (0)
#define gte_stsz3c(r0) PCGTE_SWC2_3OFF(17, 18, 19, r0, 0, 4, 8)
#define gte_stsz4c(r0)                                                                             \
	PCGTE_ST1(r0, pcgte_swc2(16, pcgte_a0_, 0); pcgte_swc2(17, pcgte_a0_, 4);                      \
	          pcgte_swc2(18, pcgte_a0_, 8); pcgte_swc2(19, pcgte_a0_, 12))
/* SZ3 / 4 as an OT index */
#define gte_stszotz(r0)                                                                            \
	PCGTE_ST1(r0, pcgte_sw(pcgte_a0_, 0, (uint32_t)((int32_t)PC_GteGetData(19) >> 2)))
#define gte_stotz(r0) PCGTE_ST1(r0, pcgte_swc2(7, pcgte_a0_, 0))
#define gte_stopz(r0) PCGTE_ST1(r0, pcgte_swc2(24, pcgte_a0_, 0))
#define gte_stlvl(r0) PCGTE_SWC2_3OFF(9, 10, 11, r0, 0, 4, 8)
#define gte_stlvnl(r0) PCGTE_SWC2_3OFF(25, 26, 27, r0, 0, 4, 8)
#define gte_stlvnl0(r0) PCGTE_ST1(r0, pcgte_swc2(25, pcgte_a0_, 0))
#define gte_stlvnl1(r0) PCGTE_ST1(r0, pcgte_swc2(26, pcgte_a0_, 0))
#define gte_stlvnl2(r0) PCGTE_ST1(r0, pcgte_swc2(27, pcgte_a0_, 0))

/* IR1..IR3 -> SVECTOR */
#define gte_stsv(r0)                                                                               \
	PCGTE_ST1(r0, uint32_t pcgte_x_ = PC_GteGetData(9); uint32_t pcgte_y_ = PC_GteGetData(10);     \
	          uint32_t pcgte_z_ = PC_GteGetData(11); pcgte_sh(pcgte_a0_, 0, pcgte_x_);             \
	          pcgte_sh(pcgte_a0_, 2, pcgte_y_); pcgte_sh(pcgte_a0_, 4, pcgte_z_))
/* IR1..IR3 -> MATRIX column */
#define gte_stclmv(r0)                                                                             \
	PCGTE_ST1(r0, uint32_t pcgte_x_ = PC_GteGetData(9); uint32_t pcgte_y_ = PC_GteGetData(10);     \
	          uint32_t pcgte_z_ = PC_GteGetData(11); pcgte_sh(pcgte_a0_, 0, pcgte_x_);             \
	          pcgte_sh(pcgte_a0_, 6, pcgte_y_); pcgte_sh(pcgte_a0_, 12, pcgte_z_))
#define gte_stbv(r0)                                                                               \
	PCGTE_ST1(r0, uint32_t pcgte_x_ = PC_GteGetData(9); uint32_t pcgte_y_ = PC_GteGetData(10);     \
	          pcgte_sb(pcgte_a0_, 0, pcgte_x_); pcgte_sb(pcgte_a0_, 1, pcgte_y_))
#define gte_stcv(r0)                                                                               \
	PCGTE_ST1(r0, uint32_t pcgte_x_ = PC_GteGetData(9); uint32_t pcgte_y_ = PC_GteGetData(10);     \
	          uint32_t pcgte_z_ = PC_GteGetData(11); pcgte_sb(pcgte_a0_, 0, pcgte_x_);             \
	          pcgte_sb(pcgte_a0_, 1, pcgte_y_); pcgte_sb(pcgte_a0_, 2, pcgte_z_))

#define gte_strgb(r0) PCGTE_ST1(r0, pcgte_swc2(22, pcgte_a0_, 0))
#define gte_strgb3(r0, r1, r2) PCGTE_SWC2_3(20, 21, 22, r0, r1, r2)
#define gte_strgb3_g3(r0) PCGTE_SWC2_3OFF(20, 21, 22, r0, 4, 12, 20)
#define gte_strgb3_gt3(r0) PCGTE_SWC2_3OFF(20, 21, 22, r0, 4, 16, 28)
#define gte_strgb3_g4(r0) PCGTE_SWC2_3OFF(20, 21, 22, r0, 4, 12, 20)
#define gte_strgb3_gt4(r0) PCGTE_SWC2_3OFF(20, 21, 22, r0, 4, 16, 28)

#define gte_ReadGeomOffset(r0, r1)                                                                 \
	do {                                                                                           \
		void *pcgte_a0_ = PCGTE_W(r0);                                                             \
		void *pcgte_a1_ = PCGTE_W(r1);                                                             \
		pcgte_sw(pcgte_a0_, 0, (uint32_t)((int32_t)PC_GteGetCtrl(24) >> 16));                      \
		pcgte_sw(pcgte_a1_, 0, (uint32_t)((int32_t)PC_GteGetCtrl(25) >> 16));                      \
	} while (0)
#define gte_ReadGeomScreen(r0) PCGTE_ST1(r0, pcgte_cfc2_sw(26, pcgte_a0_, 0))

/* control registers base..base+7 -> MATRIX (8 words) */
#define PCGTE_STMAT(base, r0)                                                                      \
	PCGTE_ST1(r0, int pcgte_i_; for (pcgte_i_ = 0; pcgte_i_ < 8; pcgte_i_++)                       \
	                  pcgte_cfc2_sw((base) + pcgte_i_, pcgte_a0_, 4 * pcgte_i_))
#define gte_ReadRotMatrix(r0) PCGTE_STMAT(0, r0)
#define gte_ReadLightMatrix(r0) PCGTE_STMAT(8, r0)
#define gte_ReadColorMatrix(r0) PCGTE_STMAT(16, r0)
#define gte_sttr(r0)                                                                               \
	PCGTE_ST1(r0, pcgte_cfc2_sw(5, pcgte_a0_, 0); pcgte_cfc2_sw(6, pcgte_a0_, 4);                  \
	          pcgte_cfc2_sw(7, pcgte_a0_, 8))
#define gte_stfc(r0)                                                                               \
	PCGTE_ST1(r0, pcgte_cfc2_sw(21, pcgte_a0_, 0); pcgte_cfc2_sw(22, pcgte_a0_, 4);                \
	          pcgte_cfc2_sw(23, pcgte_a0_, 8))
#define gte_stlzc(r0) PCGTE_ST1(r0, pcgte_swc2(31, pcgte_a0_, 0))

/* MAC1..MAC3 -> TR */
#define gte_mvlvtr()                                                                               \
	do {                                                                                           \
		uint32_t pcgte_x_ = PC_GteGetData(25), pcgte_y_ = PC_GteGetData(26),                       \
		         pcgte_z_ = PC_GteGetData(27);                                                     \
		PC_GteSetCtrl(5, pcgte_x_);                                                                \
		PC_GteSetCtrl(6, pcgte_y_);                                                                \
		PC_GteSetCtrl(7, pcgte_z_);                                                                \
	} while (0)

#define gte_nop() ((void)0)

/* Packed (x16, y16) word pairs: difference / sum of *r0 and *r1 into r2 as two longs (dvl) or two
 * shorts (dvd). The low halves pass through IR1/IR2, which they clobber, as on hardware. */
static __inline__ void pcgte_dv(const void *a, const void *b, void *out, int add, int to_short)
{
	uint32_t wa = pcgte_lw(a, 0), wb = pcgte_lw(b, 0);
	uint32_t hi, lo;
	PC_GteSetData(9, wa);
	PC_GteSetData(10, wb);
	hi = add ? (uint32_t)(((int32_t)wa >> 16) + ((int32_t)wb >> 16))
	         : (uint32_t)(((int32_t)wa >> 16) - ((int32_t)wb >> 16));
	lo = add ? PC_GteGetData(9) + PC_GteGetData(10) : PC_GteGetData(9) - PC_GteGetData(10);
	if (to_short) {
		pcgte_sh(out, 2, hi);
		pcgte_sh(out, 0, lo);
	} else {
		pcgte_sw(out, 4, hi);
		pcgte_sw(out, 0, lo);
	}
}
#define gte_subdvl(r0, r1, r2) pcgte_dv(PCGTE_P(r0), PCGTE_P(r1), PCGTE_W(r2), 0, 0)
#define gte_subdvd(r0, r1, r2) pcgte_dv(PCGTE_P(r0), PCGTE_P(r1), PCGTE_W(r2), 0, 1)
#define gte_adddvl(r0, r1, r2) pcgte_dv(PCGTE_P(r0), PCGTE_P(r1), PCGTE_W(r2), 1, 0)
#define gte_adddvd(r0, r1, r2) pcgte_dv(PCGTE_P(r0), PCGTE_P(r1), PCGTE_W(r2), 1, 1)

/* Negate the first row of the rotation matrix (RT11, RT12, RT13). */
static __inline__ void pcgte_flip_rot_x(void)
{
	uint32_t c0 = PC_GteGetCtrl(0), c1 = PC_GteGetCtrl(1);
	uint32_t a = 0u - (uint32_t)(int32_t)(int16_t)c0, b = 0u - (uint32_t)((int32_t)c0 >> 16);
	PC_GteSetCtrl(0, (a & 0xFFFF) | (b << 16));
	a = 0u - (uint32_t)(int32_t)(int16_t)c1;
	PC_GteSetCtrl(1, (a & 0xFFFF) | (c1 & 0xFFFF0000u));
}
#define gte_FlipRotMatrixX() pcgte_flip_rot_x()
#define gte_FlipTRX() PC_GteSetCtrl(5, 0u - PC_GteGetCtrl(5))

#endif

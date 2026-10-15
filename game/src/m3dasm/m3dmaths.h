/* Shared declarations for the C interpretations of the M3DMATHS / M3DASM entry points
 * (decomp/src/m3dasm/*.c). Plain C: the game headers (decomp/include/game) are C++.
 *
 * Retail ran these on the GTE; here they use the port's geometry maths (pc_geom.h), with the same
 * operations in the same order, and leave the state (pc_geom) as retail left the GTE's control
 * registers. Results retail left in the GTE's data registers are returned instead where a caller
 * reads them. */
#ifndef M3DASM_M3DMATHS_H
#define M3DASM_M3DMATHS_H

#include <string.h>
#include <sys/types.h>
#include <libgte.h>

#include "pc_geom.h"

/* M3D's plain vector/matrix structs (layouts as in decomp/include/game/vector.h and m3d.h) */
struct SVector {
	short vx, vy, vz;
};

struct Vector {
	int vx, vy, vz;
};

/* a decompressed animation frame entry: 3x3 rotation and translation, all shorts */
struct SMatrix {
	short m[3][3]; /* 0x00 */
	short t[3];    /* 0x12 */
};

/* A word made of two 16-bit halves (screen words, packed vertex x/y ...). */
static __inline__ uint32_t m3d_pack(int lo, int hi) {
	return (uint32_t)(lo & 0xffff) | ((uint32_t)hi << 16);
}

/* Several routines store m[2][2] as a whole word: the element plus the two padding bytes that
 * follow it in a MATRIX (offset 0x12). This writes that word. */
static __inline__ void m3d_store_m22_word(MATRIX *m, uint32_t w) {
	memcpy((char *)m + 16, &w, 4);
}

/* out = M * a (>> 12), each column of a through M; out takes the low 16 bits of the 32-bit sums
 * (not the saturated values). Returns the last column's full third sum, which callers store
 * together with m[2][2]'s padding. */
static __inline__ uint32_t m3d_mul_matrix_ir(const int16_t m[3][3], short a[3][3],
                                             short out[3][3]) {
	GeomAcc r;
	int c;

	for (c = 0; c < 3; c++) {
		r = geom_mvmva(m, 0, a[0][c], a[1][c], a[2][c], 12, 0, 0);
		out[0][c] = (short)r.mac[0];
		out[1][c] = (short)r.mac[1];
		out[2][c] = (short)r.mac[2];
	}

	return (uint32_t)r.mac[2];
}

/* out = M * a (>> 12), each column of a through M; out takes the saturated 16-bit results. */
/* (out of line, as it always was: with both SetSuperTransforms in m3dasm.c it got inlined) */
static __inline__ __attribute__((noinline)) void m3d_mul_matrix_v(const int16_t m[3][3],
                                                                  short a[3][3], short out[3][3]) {
	GeomAcc r;
	int c;

	for (c = 0; c < 3; c++) {
		r = geom_mvmva(m, 0, a[0][c], a[1][c], a[2][c], 12, 0, 0);
		out[0][c] = r.ir[0];
		out[1][c] = r.ir[1];
		out[2][c] = r.ir[2];
	}
}

int M3dMaths_SquareRoot0__Fi(int n);
int M3dMaths_LastVecMagnitude__Fv(void);

#endif

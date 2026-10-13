/* Shared declarations for the C interpretations of the M3DMATHS / M3DASM entry points
 * (decomp/src/m3dasm/*.c). Plain C: the game headers (decomp/include/game) are C++. */
#ifndef M3DASM_M3DMATHS_H
#define M3DASM_M3DMATHS_H

#include <sys/types.h>
#include <libgte.h>
#include <inline_c.h>

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

/* A GTE register word made of two 16-bit halves (VXY, rotation-matrix pairs ...). */
static __inline__ uint32_t m3d_pack(int lo, int hi) {
	return (uint32_t)(lo & 0xffff) | ((uint32_t)hi << 16);
}

/* Several routines store m[2][2] as a whole word: the element plus the two padding bytes that
 * follow it in a MATRIX (offset 0x12). This writes that word. */
static __inline__ void m3d_store_m22_word(MATRIX *m, uint32_t w) {
	pcgte_sw(m, 16, w);
}

/* Loads a 3x3 matrix of shorts into a GTE control matrix (0: rotation, 8: light, 16: colour),
 * the way ctc2 of the five packed words does. */
static __inline__ void m3d_load_ctrl_matrix(int base, short m[3][3]) {
	PC_GteSetCtrl(base + 0, m3d_pack(m[0][0], m[0][1]));
	PC_GteSetCtrl(base + 1, m3d_pack(m[0][2], m[1][0]));
	PC_GteSetCtrl(base + 2, m3d_pack(m[1][1], m[1][2]));
	PC_GteSetCtrl(base + 3, m3d_pack(m[2][0], m[2][1]));
	PC_GteSetCtrl(base + 4, (uint32_t)(int)m[2][2]);
}

/* The GTE matrix-times-vector commands these routines use, sf=1 (results >> 12), no translation */
enum {
	M3D_RTIR = 0x049e012, /* rotation matrix * IR */
	M3D_LCIR = 0x04de012, /* colour matrix * IR */
	M3D_RTV0 = 0x0486012, /* rotation matrix * V0 (V1, V2: + 0x8000, + 0x10000) */
	M3D_LLV0 = 0x04a6012  /* light matrix * V0 (V1, V2 likewise) */
};

/* out = M * a, where M is the GTE control matrix the command uses: each column of a goes
 * through IR1..3 and the command; out takes the low 16 bits of MAC1..3. Returns the full MAC3
 * of the last column, which callers store together with m[2][2]'s padding. */
static __inline__ uint32_t m3d_mul_matrix_ir(uint32_t cmd, short a[3][3], short out[3][3]) {
	int r, c;

	for (c = 0; c < 3; c++) {
		for (r = 0; r < 3; r++)
			PC_GteSetData(9 + r, (uint32_t)(int)a[r][c]);

		PC_GteCommand(cmd);

		for (r = 0; r < 3; r++)
			out[r][c] = (short)PC_GteGetData(25 + r);
	}

	return PC_GteGetData(27);
}

/* M * (the matrix whose columns are already in V0, V1, V2), cmd = M3D_RTV0 or M3D_LLV0:
 * out takes IR1..3 of each column. */
static __inline__ void m3d_mul_matrix_v(uint32_t cmd, short out[3][3]) {
	int r, c;

	for (c = 0; c < 3; c++) {
		PC_GteCommand(cmd + 0x8000 * c);

		for (r = 0; r < 3; r++)
			out[r][c] = (short)PC_GteGetData(9 + r);
	}
}

int M3dMaths_SquareRoot0__Fi(int n);
int M3dMaths_LastVecMagnitude__Fv(void);

#endif

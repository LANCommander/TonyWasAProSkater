/* M3DMATHS' "last vector": the working vector its routines (TransformLastVec, NormaliseLastVec,
 * CrossWithLastVec12 ...) take and leave for the next call. Retail kept it in the GTE (IR1..3,
 * with the 32-bit sums in MAC1..3); the port keeps it here. Callers (PC_PORT branches of the
 * decompiled sources) load it and read it back with these. C and C++. */
#ifndef M3DASM_LASTVEC_H
#define M3DASM_LASTVEC_H

#include "pc_geom.h"

#ifdef __cplusplus
extern "C" {
#endif

extern GeomAcc M3dMaths_LastVec;

/* the vector (three shorts: an SVECTOR's x, y, z), the sums left as they are (gte_ldsv) */
static inline void M3dMaths_SetLastVec(const void *sv)
{
	__builtin_memcpy(M3dMaths_LastVec.ir, sv, 6);
}

/* the 16-bit vector into three shorts (gte_stsv) */
static inline void M3dMaths_GetLastVec(void *sv)
{
	__builtin_memcpy(sv, M3dMaths_LastVec.ir, 6);
}

/* the 32-bit sums into three words (gte_stlvnl) */
static inline void M3dMaths_GetLastVecSums(void *v)
{
	__builtin_memcpy(v, M3dMaths_LastVec.mac, 12);
}

#ifdef __cplusplus
}
#endif

#endif
